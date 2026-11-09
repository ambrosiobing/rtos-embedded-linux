# What each language version actually changes for a cascade

Filled from the compiler's own report, in WSL on the demo laptop, not from a standards
table. The test prints which additions the compiler in front of it provided, and this page
is that log. Where the two disagree the log is right.

**Taken twice: Tuesday 6 October 2026 and again on Monday 9 November 2026.** The second run
exists because two cells of the first were wrong and because the first named no compiler,
and both of those are recorded below rather than quietly replaced.

The shape being compared matters. [P01](../../01-presence/docs/LANGUAGE_IDIOMS.md) compares
versions against a **transition table**. This project compares them against a **cascade**:
seven guarded arms and an unguarded eighth in a fixed order, as one pure function.
Different shapes reward different additions, and most of what follows is about that.

## What the compilers reported

Six combinations, all green, `-Werror -Wpedantic -Wshadow -Wconversion -fno-exceptions
-fno-rtti`.

**Monday 9 November 2026, g++ 15.2.0 and clang++ 21.1.8.** Every column is the standard it
names; neither compiler fell back.

| Feature | g++ 17 | g++ 23 | g++ 2c | clang++ 17 | clang++ 23 | clang++ 2c |
|---|---|---|---|---|---|---|
| `std::expected` | no | **yes** | **yes** | no | **yes** | **yes** |
| `std::to_underlying` | no | **yes** | **yes** | no | **yes** | **yes** |
| `consteval` | no | **yes** | **yes** | no | **yes** | **yes** |
| `std::inplace_vector` | no | no | no | no | no | no |
| `static_assert` with a computed message | no | no | **yes** | no | no | **yes** |

`__cplusplus` read 201703, 202302 and 202400 for the three standards, identically under
both compilers, in both runs.

**Tuesday 6 October 2026 differed in exactly two cells, and the difference is this page
answering two questions in one column.** It read `static_assert` with a computed message as
**yes** at clang++ 17 and clang++ 23. That was a true report of what clang provides and a
false answer to what this column is read as asking, which is whether the code here can use
it. Those two questions are exactly the ones separated by finding three below, on this same
page. The figure came from a probe in `cpp/claim26.hpp` that asked only the feature-test
macro:

    #if defined(__cpp_static_assert) && __cpp_static_assert >= 202306L

clang defines that macro in `-std=c++17` mode because it supports the feature as an
extension, so the probe was true in a translation unit that may not legally use it, and
`-Wpedantic -Werror` would refuse the syntax it unlocked. The probe now asks the macro and
`__cplusplus` together, and the two cells read no: this column reports what the code may
use, and the implementation's wider offer is finding three's subject rather than this
table's.

The same defect in P01's `presence26.hpp` turned two matrix jobs red on Monday 9 November
2026, when the runner's compiler changed. Here the build stayed green and the defect
arrived as these two cells instead, which is the worse of the two outcomes: **a red
workflow is looked at and a table is not.**

**And the first run named no compiler, which is why this page appeared to contradict
P01's.** That page reports `std::expected` as unavailable on clang at c++23 and c++2c;
this one reports it available. Both are right. P01's figures of Monday 5 October 2026 came
from the CI runner, gcc 13.3 and clang 18.1.3; this page's came from WSL on the demo
laptop a day later, whose compilers were newer. Neither table said so, and the same
omission explains the `__cplusplus` column: 202100 at 23 g++ on the runner's gcc 13.3,
202302 here. **A measured table names the instrument that produced it**, which is this
volume's rule for a meter and is no different for a compiler.

## Finding one: the compile-time proof did not need C++23

The strongest thing the C++ does is prove the six invariants of
[DESIGN.md](DESIGN.md) over the **whole input space** rather than at the twenty-two
case points: five booleans and seven service states is 224 combinations, and all of them
are checked before the program runs. The C checks the same invariants after each of
twenty-two cases, which is a sample, and a sampled invariant can hold at every point
tested and fail where nobody wrote a case.

The commit that added it said the proof would work under C++17 and that `consteval` only
adds the guarantee that it cannot run late. **The log settles that, and it was right:**
the C++17 column reports `consteval: no` and the proof line is printed anyway, because it
ran as a `constexpr` function behind a `static_assert`. Calling through a function pointer
in a constant expression has been legal since C++11, and the cascade is an array of
function pointers.

So the honest account of what the later standard buys here is narrow: `consteval` makes it
impossible for the proof to be called at run time and become a cost. That is worth having
and it is not what bought the proof.

## Finding two: `std::inplace_vector` is in neither library yet, and would be wrong anyway

Two separate facts, and the second survives the first.

**Neither libstdc++ nor libc++ provides it**, in any of the three modes, including
`-std=c++2c`. So the comparison in [claim26.hpp](../cpp/claim26.hpp) did not run on any of
the six combinations and the test says so in each, rather than passing in silence. That is
the whole purpose of reporting features from the compiler instead of from a table: a page
claiming "C++26 gives the spool a bounded vector" would have been wrong on every machine
this project can build on.

**And it is the wrong container for this spool regardless**, which is the finding that does
not depend on availability. The spool discards the **oldest** when full, because the newest
event describes the room now. In a ring that is one index step. In a vector it is
`erase(begin())`, shifting every survivor down one, with a capacity of 512. At the discard
count this project's own test produces it is roughly 450,000 element moves against 880
index increments.

Where it is right is the shape beside it: a bounded batch filled and then drained whole,
which is what a transmit batch is and has no discard-oldest to pay for. That belongs to
P11, and the type is written here because this is the chapter where the bound is argued.

## Finding three: a feature macro reports the implementation, not the standard

`clang++` reports `__cpp_static_assert >= 202306L` under **`-std=c++17`**, while `g++`
reports it only under `-std=c++2c`. The same flag, two answers, and clang's answer is yes
three standards early.

That is not a defect in either compiler. A feature-test macro says what the
implementation provides, and an implementation may provide a paper's contents before the
standard carrying it ships. What follows is practical: **a feature macro cannot be used to
infer which standard is in force**, and a project that gates behaviour on one is gating on
the compiler, not on the language version it thinks it named. Reading `__cplusplus` is the
only answer to the second question.

**And this project then did the thing this finding warns against, for five weeks.** The
paragraph above was written on Tuesday 6 October 2026 and names the remedy in its last
sentence. Both `cpp/claim26.hpp` here and `presence26.hpp` in P01 went on gating purely on
the feature macro, so on Monday 9 November 2026, when the runner's compiler changed to one
that offers more as an extension, P01's two clang jobs failed on syntax its probe had
unlocked under `-std=c++17`. The table at the top of this page had been carrying the same
mistake in two cells the whole time, visible and unread.

**A pitfall named in prose is not a gate**, which is the most repeated defect in this
volume and is recorded here because this is the clearest instance of it: the diagnosis, the
remedy and the code that ignored both were one directory apart.

This is the mirror of a P01 finding, where an edition changed what the language permitted
rather than what the library offered. Here an implementation changed what the library
offered without the standard moving at all.

## What Rust changed, and the one place it loses

Rust's three editions were added after the C and the C++, and the comparison runs in both
directions, which is the only way it is worth running.

**One branch of the C is deleted rather than translated.** `claim_service_blocks` ends with
a default arm treating an unknown service state as blocking, because a C enum can hold any
value of its underlying type and the safe answer to "is this room fit to be booked" when the
answer is unknown is no. A Rust `Service` cannot be anything but one of the seven, so the
`match` is exhaustive and the defensive arm has nothing to defend against. The deletion is
the finding: that branch is not dead code in the C, it is a real guard against a real
possibility that a different type system removes.

**`Option<fn>` costs nothing.** A function pointer cannot be null, so the `None` case uses
the niche, and the unguarded arm is genuinely `None`. The C++ needs an `always` predicate
that returns true plus a separate `guarded` bool to say the same thing, and then cannot
check the two against each other at compile time. The test asserts
`size_of::<Option<Guard>>() == size_of::<Guard>()` rather than claiming it.

**And the place Rust loses, recorded because a comparison that only finds in one direction
is not a comparison.** The C++ proves the six invariants over all 224 inputs at compile
time. Rust cannot: a `const fn` may not call through a function pointer, so the cascade
cannot be walked in a const context. The same exhaustive check runs, with the same coverage
and the same clauses, as a test. It catches the same defects and it catches them later.

**`clippy` improved on both of the earlier languages.** `manual_clamp` rejected the
two-`if` form that the C and the C++ both use for the spool bound, in favour of
`want.clamp(1, SPOOL_SLOTS)`. It is right: one expression says the bound is a range, where
two ifs say it twice and leave a reader to work out that they compose. That is the only
finding in this project so far that runs from the newest language back towards the oldest.

## A formatter can disagree with itself across editions

`rustfmt` sorts a multi-line `use` list differently under the **2024 style edition**,
putting capitalised names before lowercase ones, where 2018 and 2021 put the functions
first. Both orderings appeared in one `cargo fmt --check` run over one shared file.

So a shared source compiled under all three editions **cannot contain such a list at all**:
whatever order it is written in, one edition will reject it. The fix is a glob import, which
has no ordering to disagree about, and P01's shared test had already arrived there.

This is the third member of a family this project keeps finding. An edition changes what
the language permits; an implementation changes what the library offers without the standard
moving; and a style edition changes what the formatter requires. All three are versioned
separately, and a claim of the form "this compiles under version N" says less than it looks.

## What C++23 genuinely bought: a refusal instead of a correction

`std::expected` for settings that cannot work. The C has no settings validation at all:
`claim_spool_init` raises a bound of zero up to one event and says nothing. A device that
corrects its configuration in silence cannot be commissioned reliably, because the person
commissioning it has no way to learn they got it wrong.

Three configurations are refused, and the refusal carries which one: a grace period of
zero, which releases every booking at once; a spool bound below one event, which discards
everything in silence; and a walk-in shorter than the grace period, which frees a room
while it is in use. Under C++17 the same checks return a `bool` and the reason is lost,
which the log shows as `std::expected is absent, so the refusal is a bool here`.

The C++17 spool keeps the C's clamping behaviour on purpose, and the test asserts it does,
so the two behaviours sit side by side in one binary rather than one being quietly
replaced.
