// projects/01-presence/cpp/presence.hpp: the same twenty-nine rows, in C++17.
//
// THE BASELINE. This header compiles unchanged under -std=c++17, c++23 and c++2c,
// and that is deliberate: it uses nothing a later version added, so it is the
// control against which presence23.hpp and presence26.hpp are measured. Each of
// those adds only what its version genuinely offers this table, behind the
// feature-test macro that says the compiler actually has it, and the test prints
// what was available. "What C++23 buys" is therefore a number in a CI log rather
// than a sentence in a README.
//
// THE SAME TABLE AS ../c/presence.c, ROW FOR ROW. Same order, same guards, same
// destinations, so the row index an event takes is identical across the two
// languages and a parity test can compare them. The C is the reference; this is
// the second implementation, and agreement between the two is the evidence that
// the table and not the language is what decides.
//
// WHAT C++17 CHANGES ABOUT EXPRESSING THIS, honestly:
//
//   * enum class, so a State cannot be passed where an Event is expected, which
//     the C enums permit silently.
//   * std::variant for the event payload, replacing the C union. The union let a
//     caller read range_mm from a settings event; the variant makes that a
//     compile error or a checked access. It costs one byte of discriminant the
//     union did not carry.
//   * constexpr std::array for the table, so the row count is the array's size
//     rather than a macro that has to be asserted against it.
//   * std::string_view for names, which carries its length and costs nothing.
//   * [[nodiscard]] on dispatch, so a caller cannot silently ignore ERR_NO_ROW,
//     which is the one return that means the table is broken.
//
// WHAT IT DOES NOT CHANGE: no exceptions, no allocation, no RTTI, no virtual
// calls. Function pointers in the table, as in the C, because a std::function
// would allocate and a kernel adapter has no allocator. The dispatcher is still
// a dozen lines.
#ifndef PRESENCE_HPP
#define PRESENCE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <variant>

namespace presence {

enum class State : std::uint8_t { Free = 0, Occupied, Held, Fault };
enum class Event : std::uint8_t { Tick = 0, Reading, Timeout, Button, Fault, Settings };

constexpr std::size_t kStateCount = 4;
constexpr std::size_t kEventCount = 6;

struct Settings {
    std::uint16_t arrive_runs  = 2;
    std::uint32_t hold_ms      = 30000;
    std::uint16_t range_mm_max = 2500;
};

// The payloads. A reading carries a range; a settings event carries the
// replacement settings; everything else carries nothing. std::monostate is the
// nothing, and it is the variant's default, so an Ev{Event::Tick} is complete.
struct Reading { std::uint16_t range_mm; };

struct Ev {
    Event kind;
    std::uint32_t at_ms = 0;
    std::variant<std::monostate, Reading, Settings> payload{};
};

enum class Result : std::int8_t {
    Ok = 0,
    ErrNoRow = -2,       // the table is not total; a defect
    ErrBadState = -3,
    ErrBadEvent = -4,
};

struct Context {
    State state = State::Free;
    Settings settings{};
    std::uint16_t run = 0;
    bool hold_running = false;
    std::uint32_t hold_due_ms = 0;
    std::uint32_t last_reading_ms = 0;

    std::array<std::uint32_t, 29> row_taken{};
    std::int16_t last_row = -1;
    std::uint32_t releases = 0;
    std::uint32_t faults_latched = 0;
};

using Guard  = bool (*)(const Context&, const Ev&);
using Action = void (*)(Context&, const Ev&);

struct Row {
    State from;
    Event event;
    Guard guard;        // nullptr means unguarded, and is last among its pair
    // The same fact as `guard != nullptr`, written out by hand and duplicated on
    // purpose. The consteval order check in presence23.hpp reads this bool rather
    // than comparing the pointer, because gcc 13 under -fsanitize does not treat
    // a function-pointer comparison with nullptr as a constant expression, while
    // clang and gcc at -O2 do. The test asserts the two agree on every row, so
    // the duplication cannot drift silently.
    bool guarded;
    Action action;
    State to;
    std::string_view name;
};

// ---------------------------------------------------------------- guards
namespace detail {

inline const Reading* reading_of(const Ev& ev) {
    return std::get_if<Reading>(&ev.payload);
}

inline bool in_range(const Context& c, const Ev& ev) {
    const Reading* r = reading_of(ev);
    return r != nullptr && r->range_mm <= c.settings.range_mm_max;
}

inline bool out_of_range(const Context& c, const Ev& ev) {
    const Reading* r = reading_of(ev);
    return r != nullptr && r->range_mm > c.settings.range_mm_max;
}

inline bool run_completes(const Context& c, const Ev& ev) {
    if (!in_range(c, ev)) return false;
    const std::uint32_t next = static_cast<std::uint32_t>(c.run) + 1u;
    const std::uint32_t need = c.settings.arrive_runs ? c.settings.arrive_runs : 1u;
    return next >= need;
}

// Has the hold this timeout belongs to actually expired? Row 17 releases only when
// it has; row 18 absorbs the timeout when it has not. The reason this guard exists
// is a property of a kernel rather than of this language: no kernel can un-queue a
// timer expiry that has already fired, and a QNX channel delivers pulses in priority
// order, so an expiry from a cancelled hold can be dispatched after a NEW hold has
// started. Unguarded, row 17 would then release up to hold_ms early, which loses a
// presence as surely as never releasing. docs/RTOS_VARIANTS.md has the argument.
//
// The signed difference is deliberate and is not `ev.at_ms >= c.hold_due_ms`:
// start_hold composes the due time by adding, which wraps at 49.7 days.
inline bool hold_expired(const Context& c, const Ev& ev) {
    return static_cast<std::int32_t>(ev.at_ms - c.hold_due_ms) >= 0;
}

// ---------------------------------------------------------------- actions
inline void none(Context&, const Ev&) {}

inline void count_run(Context& c, const Ev& ev) {
    if (c.run < UINT16_MAX) ++c.run;
    c.last_reading_ms = ev.at_ms;
}
inline void reset_run(Context& c, const Ev& ev) {
    c.run = 0;
    c.last_reading_ms = ev.at_ms;
}
inline void on_arrive(Context& c, const Ev& ev) {
    c.run = 0;
    c.hold_running = false;
    c.last_reading_ms = ev.at_ms;
}
inline void refresh(Context& c, const Ev& ev) { c.last_reading_ms = ev.at_ms; }
inline void start_hold(Context& c, const Ev& ev) {
    c.hold_running = true;
    c.hold_due_ms = ev.at_ms + c.settings.hold_ms;
    c.last_reading_ms = ev.at_ms;
}
inline void cancel_hold(Context& c, const Ev& ev) {
    c.hold_running = false;
    c.last_reading_ms = ev.at_ms;
}
// Row 17. The only release in the table.
inline void release(Context& c, const Ev&) {
    c.hold_running = false;
    c.run = 0;
    ++c.releases;
}
inline void force_occupied(Context& c, const Ev&) { c.run = 0; c.hold_running = false; }
inline void force_free(Context& c, const Ev&)     { c.run = 0; c.hold_running = false; }
inline void latch_fault(Context& c, const Ev&) {
    c.hold_running = false;
    c.run = 0;
    ++c.faults_latched;
}
inline void clear_fault(Context& c, const Ev&) { c.run = 0; c.hold_running = false; }
inline void apply_settings(Context& c, const Ev& ev) {
    if (const Settings* s = std::get_if<Settings>(&ev.payload)) {
        c.settings = *s;
        if (c.settings.arrive_runs == 0) c.settings.arrive_runs = 1;
    }
}

} // namespace detail

// ------------------------------------------------------------------ table
// constexpr, so the row count is the array's size and nothing has to be
// asserted against a macro. The order is the C table's order, row for row.
inline constexpr std::array<Row, 29> kTable{{
    // 0 to 7: Free
    {State::Free, Event::Reading, detail::run_completes, true, detail::on_arrive, State::Occupied, "free reading run_completes"},
    {State::Free, Event::Reading, detail::in_range, true, detail::count_run, State::Free, "free reading in_range"},
    {State::Free, Event::Reading, detail::out_of_range, true, detail::reset_run, State::Free, "free reading out_of_range"},
    {State::Free, Event::Tick, nullptr, false, detail::none, State::Free, "free tick"},
    {State::Free, Event::Timeout, nullptr, false, detail::none, State::Free, "free timeout stale"},
    {State::Free, Event::Button, nullptr, false, detail::force_occupied, State::Occupied, "free button"},
    {State::Free, Event::Fault, nullptr, false, detail::latch_fault, State::Fault, "free fault"},
    {State::Free, Event::Settings, nullptr, false, detail::apply_settings, State::Free, "free settings"},
    // 8 to 14: Occupied
    {State::Occupied, Event::Reading, detail::in_range, true, detail::refresh, State::Occupied, "occupied reading in_range"},
    {State::Occupied, Event::Reading, detail::out_of_range, true, detail::start_hold, State::Held, "occupied reading out_of_range"},
    {State::Occupied, Event::Tick, nullptr, false, detail::none, State::Occupied, "occupied tick"},
    {State::Occupied, Event::Timeout, nullptr, false, detail::none, State::Occupied, "occupied timeout stale"},
    {State::Occupied, Event::Button, nullptr, false, detail::force_free, State::Free, "occupied button"},
    {State::Occupied, Event::Fault, nullptr, false, detail::latch_fault, State::Fault, "occupied fault"},
    {State::Occupied, Event::Settings, nullptr, false, detail::apply_settings, State::Occupied, "occupied settings"},
    // 15 to 22: Held
    {State::Held, Event::Reading, detail::in_range, true, detail::cancel_hold, State::Occupied, "held reading in_range"},
    {State::Held, Event::Reading, detail::out_of_range, true, detail::none, State::Held, "held reading out_of_range"},
    // 17 and 18 are one pair and the order is load-bearing: guarded first, so an
    // expiry belonging to this hold releases and a leftover from a cancelled hold
    // falls through to 18 and does nothing. Reversed, 18 shadows 17 and nothing
    // ever releases, which is what unguarded_rows_are_last() refuses to compile.
    {State::Held, Event::Timeout, detail::hold_expired, true, detail::release, State::Free, "held timeout release"},
    {State::Held, Event::Timeout, nullptr, false, detail::none, State::Held, "held timeout stale"},
    {State::Held, Event::Tick, nullptr, false, detail::none, State::Held, "held tick"},
    {State::Held, Event::Button, nullptr, false, detail::force_free, State::Free, "held button"},
    {State::Held, Event::Fault, nullptr, false, detail::latch_fault, State::Fault, "held fault"},
    {State::Held, Event::Settings, nullptr, false, detail::apply_settings, State::Held, "held settings"},
    // 23 to 28: Fault, a latch left only by the button
    {State::Fault, Event::Button, nullptr, false, detail::clear_fault, State::Free, "fault button"},
    {State::Fault, Event::Tick, nullptr, false, detail::none, State::Fault, "fault tick"},
    {State::Fault, Event::Reading, nullptr, false, detail::none, State::Fault, "fault reading"},
    {State::Fault, Event::Timeout, nullptr, false, detail::none, State::Fault, "fault timeout"},
    {State::Fault, Event::Fault, nullptr, false, detail::none, State::Fault, "fault fault"},
    {State::Fault, Event::Settings, nullptr, false, detail::apply_settings, State::Fault, "fault settings"},
}};

constexpr std::size_t kRowCount = kTable.size();

// ------------------------------------------------------------- dispatcher
inline void init(Context& c, const Settings* s = nullptr) {
    c = Context{};
    if (s != nullptr) c.settings = *s;
    if (c.settings.arrive_runs == 0) c.settings.arrive_runs = 1;
}

[[nodiscard]] inline Result dispatch(Context& c, const Ev& ev) {
    if (static_cast<std::size_t>(c.state) >= kStateCount) return Result::ErrBadState;
    if (static_cast<std::size_t>(ev.kind) >= kEventCount) return Result::ErrBadEvent;

    for (std::size_t i = 0; i < kRowCount; ++i) {
        const Row& row = kTable[i];
        if (row.from != c.state || row.event != ev.kind) continue;
        if (row.guard != nullptr && !row.guard(c, ev)) continue;
        row.action(c, ev);
        c.state = row.to;
        c.last_row = static_cast<std::int16_t>(i);
        ++c.row_taken[i];
        return Result::Ok;
    }
    c.last_row = -1;
    return Result::ErrNoRow;
}

[[nodiscard]] inline bool is_occupied(const Context& c) {
    return c.state == State::Occupied || c.state == State::Held;
}

[[nodiscard]] inline bool check_invariants(const Context& c) {
    if (c.hold_running && c.state != State::Held) return false;
    if (c.state == State::Held && !c.hold_running) return false;
    if (c.run != 0 && c.state != State::Free) return false;
    if (c.state == State::Fault && (c.run != 0 || c.hold_running)) return false;
    if (c.settings.arrive_runs == 0) return false;
    return true;
}

[[nodiscard]] constexpr std::string_view state_name(State s) {
    switch (s) {
    case State::Free:     return "free";
    case State::Occupied: return "occupied";
    case State::Held:     return "held";
    case State::Fault:    return "fault";
    }
    return "invalid";
}

[[nodiscard]] constexpr std::string_view event_name(Event e) {
    switch (e) {
    case Event::Tick:     return "tick";
    case Event::Reading:  return "reading";
    case Event::Timeout:  return "timeout";
    case Event::Button:   return "button";
    case Event::Fault:    return "fault";
    case Event::Settings: return "settings";
    }
    return "invalid";
}

} // namespace presence

#endif // PRESENCE_HPP
