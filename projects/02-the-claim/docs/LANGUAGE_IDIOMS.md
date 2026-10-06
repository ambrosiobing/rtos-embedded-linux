# What each language version actually changes for a cascade

Filled from the compiler's own report on Tuesday 6 October 2026, in WSL on the demo
laptop, not from a standards table. The test prints which additions the compiler in front
of it provided, and this page is that log. Where the two disagree the log is right.

The shape being compared matters. [P01](../../01-presence/docs/LANGUAGE_IDIOMS.md) compares
versions against a **transition table**. This project compares them against a **cascade**:
seven guarded arms and an unguarded eighth in a fixed order, as one pure function.
Different shapes reward different additions, and most of what follows is about that.

## What the compilers reported

Six combinations, all green, `-Werror -Wpedantic -Wshadow -Wconversion -fno-exceptions
-fno-rtti`.

| Feature | g++ 17 | g++ 23 | g++ 2c | clang++ 17 | clang++ 23 | clang++ 2c |
|---|---|---|---|---|---|---|
| `std::expected` | no | **yes** | **yes** | no | **yes** | **yes** |
| `std::to_underlying` | no | **yes** | **yes** | no | **yes** | **yes** |
| `consteval` | no | **yes** | **yes** | no | **yes** | **yes** |
| `std::inplace_vector` | no | no | no | no | no | no |
| `static_assert` with a computed message | no | no | **yes** | **yes** | **yes** | **yes** |

`__cplusplus` read 201703, 202302 and 202400 for the three standards, identically under
both compilers.

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
