// projects/01-presence/cpp/test_presence.cpp: the C test, in C++, plus the report.
//
// The same sequences as ../c/test_presence.c, row for row, so that the two
// implementations are proven against the same cases and a parity test later can
// compare the row indices they take. The cases are not restated here; the C file
// explains why each exists.
//
// What this file adds is the feature report at the end. Compiled under
// -std=c++17, c++23 and c++2c in turn, it prints which of the version-specific
// additions in presence23.hpp and presence26.hpp the compiler actually provided.
// That report, in the CI log, is the measurement docs/LANGUAGE_IDIOMS.md is built
// from. A version-specific idiom that this compiler lacks is recorded as lacking,
// not quietly compiled to the fallback and counted as present.
#include "presence26.hpp"

#include <cstdio>

using namespace presence;

static int failures = 0;
static Context ctx;

#define CHECK(cond, ...)                                                  \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("  FAIL %s:%d: ", __FILE__, __LINE__);            \
            std::printf(__VA_ARGS__);                                     \
            std::printf("\n");                                            \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

static void post(const Ev& ev, int want_row) {
    const Result r = dispatch(ctx, ev);
    CHECK(r == Result::Ok, "dispatch of %s returned %d",
          event_name(ev.kind).data(), static_cast<int>(r));
    if (want_row >= 0)
        CHECK(ctx.last_row == want_row, "expected row %d, took %d", want_row,
              static_cast<int>(ctx.last_row));
    CHECK(check_invariants(ctx), "invariant broken after %s: state=%s run=%u hold=%d",
          event_name(ev.kind).data(), state_name(ctx.state).data(),
          static_cast<unsigned>(ctx.run), static_cast<int>(ctx.hold_running));
}

static void reading(std::uint16_t mm, std::uint32_t at, int row) {
    post(Ev{Event::Reading, at, Reading{mm}}, row);
}
static void simple(Event k, std::uint32_t at, int row) { post(Ev{k, at, {}}, row); }
static void settings(std::uint16_t runs, std::uint32_t hold, std::uint16_t max, std::uint32_t at, int row) {
    post(Ev{Event::Settings, at, Settings{runs, hold, max}}, row);
}

constexpr std::uint16_t NEAR = 1000, FAR = 9000;

static void start() {
    init(ctx);
    CHECK(ctx.state == State::Free && ctx.last_row == -1 && check_invariants(ctx), "fresh context");
}

static void test_sitting() {
    std::printf("a sitting, with one ordinary gap\n");
    start();
    reading(NEAR, 100, 1); CHECK(ctx.run == 1, "run 1");
    reading(NEAR, 200, 0); CHECK(ctx.state == State::Occupied && ctx.run == 0, "arrived");
    reading(NEAR, 300, 8);
    reading(FAR, 400, 9);  CHECK(ctx.state == State::Held && is_occupied(ctx), "held reports occupied");
    reading(NEAR, 500, 15); CHECK(ctx.state == State::Occupied, "hold cancelled");
    reading(FAR, 600, 9);
    simple(Event::Timeout, 30600, 17);
    CHECK(ctx.state == State::Free && ctx.releases == 1, "released once");
}

static void test_arrival_order() {
    std::printf("arrival on the completing reading\n");
    start();
    reading(NEAR, 10, 1);
    reading(NEAR, 20, 0);
    CHECK(ctx.state == State::Occupied, "two readings arrive");
    start();
    settings(1, 1000, 2500, 5, 7);
    reading(NEAR, 10, 0);
    CHECK(ctx.state == State::Occupied, "arrive_runs 1 arrives at once");
}

static void test_only_one_release() {
    std::printf("row 17 is the only release\n");
    start();
    reading(NEAR, 10, 1); reading(NEAR, 20, 0); reading(FAR, 30, 9);
    simple(Event::Tick, 40, 19);
    reading(FAR, 50, 16);
    settings(2, 30000, 2500, 60, 22);
    CHECK(ctx.state == State::Held && ctx.releases == 0, "nothing but timeout releases");
    simple(Event::Timeout, 30030, 17);   // the hold began at 30
    CHECK(ctx.releases == 1, "the timeout releases");
}

static void test_stale_timer() {
    std::printf("a stale timer in occupied and in free\n");
    start();
    reading(NEAR, 10, 1); reading(NEAR, 20, 0); reading(FAR, 30, 9); reading(NEAR, 40, 15);
    simple(Event::Timeout, 50, 11);
    CHECK(ctx.state == State::Occupied && ctx.releases == 0, "stale timeout ignored");
    reading(FAR, 60, 9);
    simple(Event::Button, 70, 20);
    simple(Event::Timeout, 80, 4);
    CHECK(ctx.state == State::Free, "stale timeout in free ignored");
}

// The third case of the same phenomenon and the reason row 17 carries a guard. The
// two above arrive in a state that is not holding, so an unguarded row absorbs them.
// This one arrives while a NEWER hold runs, where an unguarded row 17 would release
// it up to hold_ms early. Remove the guard from the table and the first CHECK below
// fails: the room goes free with somebody in it. docs/RTOS_VARIANTS.md draws the
// sequence and names the kernel that can deliver it, which is QNX, by priority.
static void test_a_stale_timer_cannot_release_a_newer_hold() {
    std::printf("a stale hold timer arriving while a newer hold runs
");
    start();
    reading(NEAR, 10, 1); reading(NEAR, 20, 0);
    reading(FAR, 30, 9);                  // first hold, due at 30030
    CHECK(ctx.hold_due_ms == 30030, "the first hold is due at 30030");
    reading(NEAR, 40, 15);                // cancelled, expiry already queued
    reading(FAR, 50, 9);                  // second hold, due at 30050
    CHECK(ctx.state == State::Held && ctx.hold_due_ms == 30050, "second hold due at 30050");
    simple(Event::Timeout, 30030, 18);    // the first hold's expiry, stamped when it fired
    CHECK(ctx.state == State::Held, "a stale expiry must not end a running hold");
    CHECK(ctx.releases == 0 && ctx.hold_running, "and must not release");
    simple(Event::Timeout, 30050, 17);    // the real one still works
    CHECK(ctx.state == State::Free && ctx.releases == 1, "the newer hold does release");
}

static void test_fault_latch() {
    std::printf("a fault leaves only by the button\n");
    start();
    simple(Event::Fault, 10, 6);
    simple(Event::Tick, 20, 24);
    reading(NEAR, 30, 25);
    CHECK(ctx.state == State::Fault, "a good reading must not clear a fault");
    simple(Event::Timeout, 40, 26);
    simple(Event::Fault, 50, 27);
    settings(2, 30000, 2500, 60, 28);
    simple(Event::Button, 70, 23);
    CHECK(ctx.state == State::Free, "the button clears it");
}

static void test_remaining() {
    std::printf("the remaining rows\n");
    start();
    reading(FAR, 10, 2);
    simple(Event::Tick, 20, 3);
    simple(Event::Button, 30, 5);
    simple(Event::Tick, 40, 10);
    settings(2, 30000, 2500, 50, 14);
    simple(Event::Fault, 60, 13);
    simple(Event::Button, 70, 23);
    reading(NEAR, 80, 1); reading(NEAR, 90, 0);
    simple(Event::Button, 100, 12);
    reading(NEAR, 110, 1); reading(NEAR, 120, 0); reading(FAR, 130, 9);
    simple(Event::Fault, 140, 21);
    CHECK(ctx.state == State::Fault, "fault from held");
}

static void test_refusals() {
    std::printf("what the dispatcher refuses\n");
    start();
    Ev bad{static_cast<Event>(kEventCount), 0, {}};
    CHECK(dispatch(ctx, bad) == Result::ErrBadEvent, "bad event refused");
    start();
    ctx.state = static_cast<State>(kStateCount);
    CHECK(dispatch(ctx, Ev{Event::Tick, 0, {}}) == Result::ErrBadState, "bad state refused");
    // A reading event with no payload is not in range and not out of range, so
    // in Free it matches no guarded row. The table has no unguarded Free+Reading
    // row on purpose, so this is ErrNoRow, and that is correct: a reading with no
    // range is a malformed event, not a transition.
    start();
    CHECK(dispatch(ctx, Ev{Event::Reading, 0, {}}) == Result::ErrNoRow,
          "a reading with no payload matches no row");
}

#if PRESENCE_HAS_EXPECTED
static void test_expected_return() {
    std::printf("C++23: the row index is the return value\n");
    start();
    auto r = v23::dispatch(ctx, Ev{Event::Tick, 0, {}});
    CHECK(r.has_value() && *r == 3, "tick in free is row 3 as a value");
    auto bad = v23::dispatch(ctx, Ev{Event::Reading, 0, {}});
    CHECK(!bad.has_value() && bad.error() == Result::ErrNoRow, "the error is the alternative");
}
#endif

static void test_queue() {
    std::printf("C++26: the sixteen-deep queue, or its fallback\n");
    v26::Queue q{};
    for (std::uint32_t i = 0; i < v26::kQueueDepth; ++i)
        CHECK(v26::enqueue(q, Ev{Event::Tick, i, {}}), "slot %u accepted", static_cast<unsigned>(i));
    CHECK(q.size() == v26::kQueueDepth, "full at sixteen");
    CHECK(!v26::enqueue(q, Ev{Event::Tick, 99, {}}), "the seventeenth is refused, not dropped silently");
}

// Row::guarded duplicates `guard != nullptr` by hand so that the consteval order
// check in presence23.hpp can read a bool, after gcc 13 under the sanitisers
// refused the pointer comparison as a constant expression. A duplicated fact
// drifts unless something checks it, and this is that something: the pointer
// comparison is fine at run time, so every row's bool is held against it here.
static void test_guarded_flag_agrees_with_the_pointer() {
    std::printf("Row::guarded agrees with the guard pointer on every row\n");
    for (std::size_t i = 0; i < kRowCount; ++i)
        CHECK((kTable[i].guard != nullptr) == kTable[i].guarded,
              "row %zu: guarded=%d but the pointer says %d", i,
              static_cast<int>(kTable[i].guarded),
              static_cast<int>(kTable[i].guard != nullptr));
}

static std::uint32_t coverage[kRowCount];
static void accumulate() { for (std::size_t i = 0; i < kRowCount; ++i) coverage[i] += ctx.row_taken[i]; }

int main() {
    void (*cases[])() = {test_sitting, test_arrival_order, test_only_one_release,
                         test_stale_timer, test_a_stale_timer_cannot_release_a_newer_hold,
                         test_fault_latch, test_remaining, test_refusals,
                         test_guarded_flag_agrees_with_the_pointer,
#if PRESENCE_HAS_EXPECTED
                         test_expected_return,
#endif
                         test_queue};
    for (auto fn : cases) { fn(); accumulate(); }

    std::printf("every row reachable\n");
    unsigned untaken = 0;
    for (std::size_t i = 0; i < kRowCount; ++i)
        if (coverage[i] == 0) { std::printf("  FAIL row %zu never taken\n", i); ++untaken; ++failures; }
    if (untaken == 0) std::printf("  all %zu rows taken\n", kRowCount);

    // The measurement this file exists to produce.
    const auto f = v26::features();
    std::printf("\nfeature report, __cplusplus=%ld\n", f.cplusplus);
    std::printf("  std::expected as the dispatch return   %s\n", f.expected ? "available" : "NOT available, baseline used");
    std::printf("  std::to_underlying                     %s\n", f.to_underlying ? "available" : "NOT available, static_cast used");
    std::printf("  consteval totality and order checks    %s\n", f.consteval_checks ? "available, enforced at compile time" : "NOT available, run-time test only");
    std::printf("  std::inplace_vector for the queue      %s\n", f.inplace_vector ? "available" : "NOT available, array-and-count fallback");
    std::printf("  static_assert with a built message     %s\n", f.static_assert_message ? "available" : "NOT available");
    std::printf("  = delete(\"reason\")                     %s\n", f.delete_with_reason ? "available" : "NOT available");
    std::printf("  pack indexing                          %s (no use in this table)\n", f.pack_indexing ? "available" : "NOT available");

    // What the safer payload costs, as a number rather than as a sentence. The C
    // event is a union of the same two payloads and is pinned at 20 bytes by a
    // _Static_assert; std::variant adds a discriminant and whatever alignment it
    // needs. Printed for all three standards, because a later standard changing
    // the layout would be worth knowing and is not something to assume either way.
    std::printf("\nsize report, the cost of the payload being checked\n");
    std::printf("  Settings                               %zu B\n", sizeof(Settings));
    std::printf("  Reading                                %zu B\n", sizeof(Reading));
    std::printf("  Ev, payload as std::variant            %zu B (the C union is 20)\n", sizeof(Ev));
    std::printf("  Context                                %zu B (the C is 152)\n", sizeof(Context));

    std::printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
