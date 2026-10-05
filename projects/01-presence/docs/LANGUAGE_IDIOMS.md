# What each language version actually buys this table

The question this page answers is narrow on purpose: for a 28-row transition table
with a dozen-line dispatcher, compiled with no allocator and no exceptions, what
does moving to a newer version of the language change? Not what the release notes
list. What changes in **this** code.

The answer is measured rather than asserted. The same table is compiled under each
version in `code.yml`, every version-specific addition sits behind its feature-test
macro, and the test prints a report of which additions the compiler actually
provided. The columns marked *reported by CI* below are filled from that log, not
from a standards table, and they say which compiler produced them.

## C: C11, C17, C23

One source file, `c/presence.c`, under all three. No per-version source, no
conditional compilation.

| Version | What it changes for this table | Verdict |
|---|---|---|
| C11 | `_Static_assert` pins the row count to the table's real length; `stdbool.h`, designated initialisers, anonymous unions in the event | the baseline, and everything the table needs |
| C17 | nothing. C17 is a defect-fix release with no new language features | compiles identically; the point of including it is to show that |
| C23 | `bool`, `true` and `false` as keywords, `nullptr`, `constexpr` for object constants, `[[nodiscard]]` as a standard attribute, `static_assert` without the underscore, `enum` with a fixed underlying type | nothing the C11 source needed; the same source compiles because C23 kept C11 valid |

**The finding:** for a table of function pointers and an enum of states, C23 adds
nothing the C11 source lacked. A C23-only rewrite would use `nullptr` in place of
`NULL` and `constexpr` for the defaults, both of which read better and generate the
same code. That is worth knowing because the usual claim is the reverse.

## C++: C++17, C++23, C++26

Three headers, layered. `presence.hpp` is the C++17 baseline and compiles unchanged
under all three. `presence23.hpp` and `presence26.hpp` add only what their version
offers, behind feature-test macros, so a compiler lacking a feature compiles to the
baseline and the test says so.

| Version | Addition | What it changes here | Reported by CI |
|---|---|---|---|
| C++17 | `enum class` | a `State` cannot be passed where an `Event` is expected, which the C enums permit silently | baseline |
| C++17 | `std::variant` for the payload | the C union let a caller read a range from a settings event; the variant makes that a checked access. Costs one byte of discriminant | baseline |
| C++17 | `constexpr std::array` table | the row count is the array's size, so no macro to assert against | baseline |
| C++17 | `[[nodiscard]]` on `dispatch` | a caller cannot silently ignore `ErrNoRow`, the one return meaning the table is broken | baseline |
| C++23 | `std::expected<size_t, Result>` | the row index is the return value and the error the alternative, so a caller cannot read `last_row` after a failure and mistake the previous row for this one. This is a real class of defect removed | see the log |
| C++23 | `consteval` totality check | a missing `(state, event)` pair is a compile error rather than a run-time `ErrNoRow`. The C proves totality by a test; this refuses to compile without it | see the log |
| C++23 | `consteval` guard-order check | an unguarded row placed before a guarded one it would shadow is a compile error | see the log |
| C++23 | `std::to_underlying` | replaces a `static_cast`; reads better, same code | see the log |
| C++26 | `std::inplace_vector<Ev, 16>` | the chapter's sixteen-deep queue with no allocator, which before this was an array plus a hand-written count and the bugs that come with it. **The single most useful thing a new standard has handed an embedded queue in years** | see the log |
| C++26 | `static_assert` with a built message | the totality failure names the row count rather than saying a pair is missing | see the log |
| C++26 | `= delete("reason")` | copying a `Context` refuses with a sentence saying why: one node has one context | see the log |
| C++26 | pack indexing | **no use in this table**, listed so the absence is on record | see the log |

**The findings so far, before the first CI run:** C++17 earns its place through
`enum class` and `std::variant`, both of which turn a silent misuse into a compile
error. C++23's `std::expected` removes a real defect class and its `consteval`
checks move totality from a test to the compiler. C++26's `inplace_vector` is the
one addition that changes what the kernel adapter has to write.

What C++26 does **not** yet offer on any compiler the CI runner has: reflection,
which would let the Mermaid diagram be generated from the table at compile time,
and contracts, which would let the invariant be a precondition rather than a
function. Both are in the standard. Neither is in the toolchain. That gap is the
most important fact on this page, and it is why every C++26 idiom here is probed
rather than assumed.

## Rust: editions 2018, 2021, 2024

Not started. The section is here so the shape of the page is complete and so a
reader does not infer that Rust was tried and omitted.

## How to read the CI log

Each `code.yml` job prints a block like this at the end of the test:

```text
feature report, __cplusplus=202302
  std::expected as the dispatch return   available
  consteval totality and order checks    available, enforced at compile time
  std::inplace_vector for the queue      NOT available, array-and-count fallback
```

The job name carries the standard flag and the compiler. The report carries what
that compiler provided. The two together are the measurement, and this page is
updated from them rather than the other way round.
