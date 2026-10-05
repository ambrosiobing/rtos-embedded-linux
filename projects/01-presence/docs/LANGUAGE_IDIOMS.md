# What each language version actually buys this table

The question this page answers is narrow on purpose: for a 28-row transition table
with a dozen-line dispatcher, compiled with no allocator and no exceptions, what
does moving to a newer version of the language change? Not what the release notes
list. What changes in **this** code.

The answer is measured rather than asserted. The same table is compiled under each
version in `code.yml`, every version-specific addition sits behind its feature-test
macro, and the test prints a report of which additions the compiler actually
provided. **Every result on this page comes from those logs**, which are summarised
under Measured below; the two tables disagree with each other nowhere, and where
the measurements contradicted an earlier claim of mine the claim is struck and the
measurement kept.

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
| C++23 | `std::expected<size_t, Result>` | the row index is the return value and the error the alternative, so a caller cannot read `last_row` after a failure and mistake the previous row for this one. A real class of defect removed | **g++ 13.3 only; absent on clang 18 at every flag** |
| C++23 | `consteval` totality check | a missing `(state, event)` pair is a compile error rather than a run-time `ErrNoRow`. The C proves totality by a test; this refuses to compile without it | available on both, at 23 and 26 |
| C++23 | `consteval` guard-order check | an unguarded row placed before a guarded one it would shadow is a compile error | available on both; **reads a bool, not a pointer, see finding 3** |
| C++23 | `std::to_underlying` | replaces a `static_cast`; reads better, same code | available on both, at 23 and 26 |
| C++26 | `std::inplace_vector<Ev, 16>` | the chapter's sixteen-deep queue with no allocator, which before this was an array plus a hand-written count and the bugs that come with it. Potentially the most useful thing a recent standard offers an embedded queue, **and that is a prediction, not a result: see finding 3** | **not available on any compiler; the fallback is what ran** |
| C++26 | `static_assert` with a built message | the totality failure names the row count rather than saying a pair is missing | **clang 18 at `c++2c` only**, the one genuinely new cell |
| C++26 | `= delete("reason")` | copying a `Context` refuses with a sentence saying why: one node has one context | **not available anywhere; never compiled in its intended form** |
| C++26 | pack indexing | **no use in this table**, listed so the absence is on record | not available anywhere either |

**What survives measurement:** C++17 earns its place through `enum class` and
`std::variant`, both of which turn a silent misuse into a compile error, and both
are unconditional. C++23's `consteval` checks move totality and guard order from a
run-time test to the compiler, on both compilers. Those three are the page's solid
ground.

**What does not:** C++26 contributed exactly one cell of twelve, on one compiler.
Three of its four idioms here were available nowhere, so the code written for them
has never compiled in its intended form and every claim made for them is a
prediction. Reflection and contracts, which would generate the diagram from the
table and make the invariant a precondition, are in the standard and in neither
toolchain. **The gap between what a standard contains and what a compiler provides
is the finding of this page**, and it is why every idiom here is probed and
reported rather than assumed.

## Measured, Sunday 5 October 2026

Two runs. The first was twelve of fifteen green and the three failures are below.
The second, after the fixes, was fifteen of fifteen, and its logs are where every
number on this page comes from.

**The compilers, read from the log rather than inferred:**

| | Version |
|---|---|
| gcc and g++ | `gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` |
| clang and clang++ | `Ubuntu clang version 18.1.3 (1ubuntu1)` |

### C: the flag, and nothing else

| Job | Flag actually used |
|---|---|
| C (c11) | `-std=c11` |
| C (c17) | `-std=c17` |
| C (c23) | **`-std=c2x`**, because gcc 13.3 does not know `c23` |
| C via clang, all three | as asked, including `-std=c23` |

The same two source files compiled and the row-coverage test passed under every
one. **C17 and C23 changed nothing about this table**, which was the prediction
and is now a result.

### C++: the twelve feature reports

Each cell is what that compiler at that standard actually provided. This is the
table the page exists for.

| Idiom | 17 g++ | 17 clang | 23 g++ | 23 clang | 26 g++ | 26 clang |
|---|---|---|---|---|---|---|
| `__cplusplus` | 201703 | 201703 | **202100** | **202302** | 202100 | **202400** |
| `std::expected` return | no | no | **yes** | **no** | **yes** | **no** |
| `std::to_underlying` | no | no | yes | yes | yes | yes |
| `consteval` checks | no | no | yes | yes | yes | yes |
| `std::inplace_vector` | no | no | no | no | **no** | **no** |
| `static_assert` message | no | no | no | no | no | **yes** |
| `= delete("reason")` | no | no | no | no | **no** | **no** |
| pack indexing | no | no | no | no | **no** | **no** |

The 26 g++ column is `-std=c++23`, because gcc 13.3 has no C++26 mode at all and
the probe fell back. That is why its numbers are identical to the 23 g++ column.

### Four findings, and two of them correct what this page previously claimed

**1. Availability is the compiler's and the library's jointly, not the standard's,
and the newer compiler can have less.** `std::expected` is available on g++ 13.3
at `c++23` and **not** on clang 18.1.3 at `c++23` or at `c++2c`. So the one idiom
here that removes a real defect class has only ever been exercised by one of the
two compilers, and the `C++23: the row index is the return value` test is silently
skipped on clang. Why clang with the same libstdc++ does not expose the macro is
**not established** and would need checking before it is asserted; the observation
is recorded, the mechanism is not.

**2. gcc 13.3 at `-std=c++23` is the draft, not the standard.** It reports
`__cplusplus=202100`; clang 18 at the same flag reports `202302`, the ratified
value. A project that gates on `__cplusplus >= 202302L` would silently take its
pre-C++23 path on gcc 13 while the job name said C++23. This is the strongest
argument on the page for probing features rather than versions.

**3. `std::inplace_vector` was not available anywhere, so the claim made for it
here has never been tested.** This page previously called it "the single most
useful thing a new standard has handed an embedded queue in years". That may still
be true and it is now explicitly **unverified**: in all twelve jobs the
array-and-count fallback is what compiled and what the queue test exercised. The
same applies to `= delete("reason")` and to pack indexing, neither of which was
available on any compiler. **Three of the four C++26 idioms probed here have never
run in their intended form.**

**4. One idiom of twelve cells was genuinely new.** `static_assert` with a built
message, on clang++ at `-std=c++2c` only.

### The three failures from the first run

**gcc 13.3 spells C23 as `-std=c2x`.** It rejected `-std=c23` with "did you mean
-std=c2x", while clang 18 accepted it. Same standard, pre-release name. The
workflow now probes and reports which flag it used.

**gcc 13.3 has no C++26 mode.** Neither `-std=c++2c` nor `-std=c++26`; it
suggested `c++20`. The probe falls back to `c++23` and the log says so in a
sentence, so a green badge on that job cannot be read as C++26 support.

**gcc 13.3 under the sanitisers will not compare a function pointer with
`nullptr` in a constant expression.** The `consteval` guard-order proof did
exactly that and gcc reported
`'(presence::detail::run_completes != 0)' is not a constant expression` at
`-O1 -g -fsanitize=address,undefined`, while the same line compiled at `-O2` and
under clang at every setting. The proof now reads a hand-written `bool guarded`,
and the runtime test holds that bool against the pointer on all 28 rows so the
duplication cannot drift. **This is the only one of the three that was in the
source**, and it was found by a check that exists to prove a claim in `DESIGN.md`.

### What a reader should take from this

The version a job is named after is not the version it compiled. Of the eight
rows in the matrix, two behave differently between compilers at the same flag, one
behaves differently from its own job name, and three were never available at all.
Every one of those would have been invisible in a page written from release notes,
and every one is visible in a log because each idiom is probed and reported rather
than assumed.

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
