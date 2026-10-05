// projects/01-presence/cpp/presence23.hpp: what C++23 adds to the same table.
//
// Nothing here replaces presence.hpp. It layers on top of it, and every addition
// is behind the feature-test macro that says the compiler actually provides it.
// A compiler that lacks the feature compiles this header to the baseline, and the
// test reports which branch was taken. So the claim "C++23 buys this" is checked
// by the compiler that is making it, which is the only honest way to make it.
//
// TWO THINGS C++23 GENUINELY CHANGES FOR THIS TABLE, and one it does not.
//
//   1. std::expected<std::size_t, Result> as the dispatch return. The C and the
//      C++17 dispatcher return an error code and write the row index into the
//      context as a side effect. Expected makes the row index the return value
//      and the error the alternative, so a caller cannot read last_row after a
//      failed dispatch and mistake the previous row for this one. That is a real
//      class of defect, and this is the version that removes it.
//
//   2. A consteval proof that the table is total. consteval is C++20, but the
//      pairing with a constexpr std::array table and the habit of writing such
//      checks is what a C++23 codebase does, and it turns the totality claim in
//      DESIGN.md from a comment into a compile error when it stops being true.
//      The C version proves totality by a test at run time; this one refuses to
//      compile a table with a missing pair. Both are correct. One is earlier.
//
//   And the one it does not: std::to_underlying replaces a static_cast on the
//   enum, which reads better and changes nothing about the code generated.
//
// Everything stays allocation free and exception free. std::expected carries its
// value or error inline; it does not allocate.
#ifndef PRESENCE23_HPP
#define PRESENCE23_HPP

#include "presence.hpp"

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
#define PRESENCE_HAS_EXPECTED 1
#else
#define PRESENCE_HAS_EXPECTED 0
#endif

#if defined(__cpp_lib_to_underlying) && __cpp_lib_to_underlying >= 202102L
#include <utility>
#define PRESENCE_HAS_TO_UNDERLYING 1
#else
#define PRESENCE_HAS_TO_UNDERLYING 0
#endif

#if defined(__cpp_consteval) && __cpp_consteval >= 201811L
#define PRESENCE_HAS_CONSTEVAL 1
#else
#define PRESENCE_HAS_CONSTEVAL 0
#endif

namespace presence::v23 {

// ------------------------------------------------- totality, at compile time
#if PRESENCE_HAS_CONSTEVAL
// Every (state, event) pair has at least one row. Evaluated by the compiler, so
// deleting a row from kTable is a compile error here rather than a run-time
// ERR_NO_ROW in the field.
consteval bool table_is_total() {
    for (std::size_t s = 0; s < kStateCount; ++s) {
        for (std::size_t e = 0; e < kEventCount; ++e) {
            bool found = false;
            for (const Row& r : kTable) {
                if (static_cast<std::size_t>(r.from) == s &&
                    static_cast<std::size_t>(r.event) == e) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
    }
    return true;
}
static_assert(table_is_total(), "a (state, event) pair has no row: the table is not total");

// And the pair that is told apart only by guard order: an unguarded row must be
// last among the rows sharing its state and event, or it would shadow the
// guarded ones after it. Also a compile-time check.
//
// This reads Row::guarded, a bool, and not `guard != nullptr`. The first CI run
// found that gcc 13 compiling with -fsanitize=address,undefined refuses to treat
// a function pointer compared with nullptr as a constant expression, with
//     error: '(presence::detail::run_completes != 0)' is not a constant expression
// while the same line is accepted by gcc at -O2 and by clang at every setting.
// So the compile-time proof compares a bool that is always a constant, and the
// runtime test proves the bool agrees with the pointer on every row.
consteval bool unguarded_rows_are_last() {
    for (std::size_t i = 0; i < kRowCount; ++i) {
        if (kTable[i].guarded) continue;
        for (std::size_t j = i + 1; j < kRowCount; ++j) {
            if (kTable[j].from == kTable[i].from && kTable[j].event == kTable[i].event)
                return false;
        }
    }
    return true;
}
static_assert(unguarded_rows_are_last(), "an unguarded row shadows a guarded one after it");
#endif

// ------------------------------------------------ expected as the return
#if PRESENCE_HAS_EXPECTED
// The row index taken, or the reason none was. The context's last_row is still
// written for parity with the C, but a caller that reads the return value cannot
// confuse a failed dispatch with the previous success.
[[nodiscard]] inline std::expected<std::size_t, Result> dispatch(Context& c, const Ev& ev) {
    const Result r = presence::dispatch(c, ev);
    if (r != Result::Ok) return std::unexpected(r);
    return static_cast<std::size_t>(c.last_row);
}
#endif

#if PRESENCE_HAS_TO_UNDERLYING
[[nodiscard]] constexpr auto raw(State s) { return std::to_underlying(s); }
[[nodiscard]] constexpr auto raw(Event e) { return std::to_underlying(e); }
#else
[[nodiscard]] constexpr auto raw(State s) { return static_cast<std::uint8_t>(s); }
[[nodiscard]] constexpr auto raw(Event e) { return static_cast<std::uint8_t>(e); }
#endif

} // namespace presence::v23

#endif // PRESENCE23_HPP
