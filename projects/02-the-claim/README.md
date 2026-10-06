# P02. The claim, and the service axis

Status on Tuesday 6 October 2026: **the cascade is written in C and in C++, and green in
twelve compiler and standard combinations** in WSL on the demo laptop. C11, C17 and C23
under gcc and clang; C++17, C++23 and C++2c under g++ and clang++; all at `-Werror` with
`-Wpedantic -Wshadow -Wconversion`, the C++ additionally at `-fno-exceptions -fno-rtti`,
and both under the address and undefined-behaviour sanitisers. All seven of chapter 02's
acceptance criteria pass in both languages, and the twenty-two cases reach all seven claim
codes, all eight arms, all seven service states and all seven service reasons.

The design page is [docs/DESIGN.md](docs/DESIGN.md) and it was committed **before any
code**, which is chapter 02's first requirement and the reason the git history is the
evidence for it rather than this sentence.

The written design is [chapter 02](../../chapters/02-the-claim.md), which is complete.
What is absent is Rust, the cross-check and the kernels.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, and nothing else |
| Board | NUCLEO-H7A3ZI-Q alone. The user button stands in for a panel button |
| Peripherals | The three indicators, now driven by the claim rather than by presence; the virtual console; two kernel timers, for the grace period and for the longer hold |
| Toolchain | As P01, plus the scripted-event runner |
| Operating system | One thread, the same queue discipline as P01 |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What this project is, and how it differs in shape from P01

P01 decides whether a room is in use. This one decides whether the room is **claimed**,
which is a different question: presence is an observation, and a claim is a decision.
P02 **adds no state to the presence machine**.

So the specification has a different shape. P01's is a transition table, because presence
is a state machine. This one's is a **cascade**: seven guarded arms and an
unguarded eighth, evaluated top to bottom, first match winning, as one pure function.
The seven produce chapter 02's seven claim codes and the eighth is the fall-through, the
absence of a claim. A second axis of seven service states runs beside all of it, neither
axis being a state of the other.

The two properties the design exists to protect are that **a live booking beats a
walk-in while a booking nobody turned up for does not**, and that **a room which cannot
be booked must not report itself free**.

## What exists

| Part | State |
|---|---|
| [docs/DESIGN.md](docs/DESIGN.md) | the cascade in order, the two axes, the six invariants, the spool arithmetic, and a Mermaid diagram of the arms |
| [c/claim.h](c/claim.h) | seven codes and the fall-through, seven service states, seven reasons, the settings and the spool |
| [c/claim.c](c/claim.c) | the eight arms as one pure function, and a ring bounded in bytes |
| [c/test_claim.c](c/test_claim.c) | the twenty-two cases as a table, and chapter 02's seven criteria as seven named tests |
| [cpp/claim.hpp](cpp/claim.hpp) | the same eight arms as a `constexpr` array, the C++17 baseline, compiling unchanged under all three standards |
| [cpp/claim23.hpp](cpp/claim23.hpp) | the six invariants proved over all 224 inputs, and `std::expected` for settings that are refused rather than corrected |
| [cpp/claim26.hpp](cpp/claim26.hpp) | `std::inplace_vector` as a bounded batch, and the same spool written the wrong way on purpose so the cost can be compared |
| [cpp/test_claim.cpp](cpp/test_claim.cpp) | the same twenty-two cases and the same seven criteria, plus two more tests and the feature report |
| [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md) | what each version changed for a cascade, filled from the compiler's own report |
| [docs/figures](docs/figures) | the chapter's five figures, as rendered SVG |
| Rust, the cross-check, the adapter and the kernels | **not written** |

## What the C++ proves at compile time that the C asserts at run time

The C writes the cascade as a chain of named predicates. The C++ writes it as a
`constexpr std::array` of `{guard, code}`. Both are the same ordered policy with the same
seven guard names, which is what a comparison is of; the array is the form that can be
reasoned about before the program runs, and three of the claims become `static_assert`s:

- **the only unguarded arm is the last one**, or it would shadow every arm below it
- **every arm carries a guard pointer**, with the duplicated `guarded` flag checked
  against it, exactly as P01's `Row::guarded` is
- **every code is produced by some arm**, so no code exists that the policy cannot reach

All three hold under C++17 with nothing but `std::array`. That matters for the comparison
this project is making: the interesting question is not what a later standard allows but
what the earliest one already did, and the answer here is more than expected.

## What the run prints, because the page should not be the source of a number

    22 cases, 7 claim codes and the fall-through, 7 service states, 7 reasons
    4400 steps, 1399 events, 512 retained, 887 discarded
    event 8 bytes, bound 4096 bytes, so capacity 512 events
    offered 1000, retained 512, discarded 488

The event is fixed at 8 bytes by a `_Static_assert`, so the 4096-byte bound buying 512
events is a compile-time fact and not an arithmetic claim. The spool lines are the
chapter's seventh criterion: retained plus discarded equals offered, and the **oldest**
went first, which a ring discarding the newest would also satisfy on the counts alone
while keeping a record of a room that has moved on.

## What the C++ found that the C could not

Three things, each checked by the compiler making the claim rather than asserted on a page.
The full account is in [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md).

**The invariants are proved over all 224 inputs, not the 22 cases.** Five booleans and
seven service states is the entire input space, and a constant evaluator walks it before
the program runs. A sampled invariant can hold at every point tested and fail where nobody
wrote a case; an exhausted one cannot. Every one of the eight arms is also proved reachable,
so an arm cannot become dead code in the shape of a policy.

**That proof did not need C++23**, which the commit adding it predicted and the log
confirmed: the C++17 column reports `consteval: no` and prints the proof anyway, because
calling through a function pointer in a constant expression has been legal since C++11.
What the later standard adds is that a proof cannot be called at run time and become a cost.

**`std::inplace_vector` is absent from both libraries and would be wrong here anyway.**
Discarding the oldest is one index step in a ring and `erase(begin())` in a vector, which
shifts all 512 survivors: roughly 450,000 element moves against 880 at this project's own
discard count. Both forms are written and the test requires identical retained contents, so
it is a comparison rather than an opinion.

## What is planned, in the order P01 established

That order is not a preference. It is what made P01's claims checkable, and each step
exists to catch something the step before it cannot.

1. ~~The C cascade and its host test.~~ **Done and green.** The invariants are checked
   after every case rather than at the end, because a broken invariant can be transient
   and still wrong.
2. ~~The same cascade in C++17, C++23 and C++2c.~~ **Done and green**, six combinations,
   with the version-specific headers and a feature report. Three findings came out of it
   and all three are in [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md): the
   whole-input-space proof did not need C++23, `std::inplace_vector` is in neither library
   yet and would be the wrong container anyway, and a feature macro reports the
   implementation rather than the standard.
3. **The same cascade in Rust**, `no_std`, under editions 2018, 2021 and 2024 from one
   source.
4. **A cross-check script** comparing the three cascades arm for arm, so that "the same
   policy" is enforced in CI rather than repeated by hand. P01 has one for its table;
   this project does not yet.
5. **The adapter contract and the shared phases**, including no kernel header, so that
   one test runs against every kernel instead of each kernel having its own.
6. **The FreeRTOS and Zephyr adapters**, both of which now have a working local loop and
   a CI job in P01, so neither is new ground.

## What will not be known even when all of that is green

**There is no calendar.** Window messages arrive as scripted input on the host and as
typed commands on the board. No chapter in this volume integrates with a calendar
service and none claims to.

**There is no panel.** The user button stands in for one, which exercises the two events
a panel actually produces.

**There is no transport.** Events are spooled and counted, not sent. P11 carries them.

**Nothing will have run on a board.** That is true of every project in this volume so
far, and for this chapter it matters less than it looks: every acceptance criterion in
chapter 02 is a scripted case on a host, which for a chapter about a decision is
stronger evidence than a demonstration on a desk. Any timing figure taken on a host is a
measurement of the host.
