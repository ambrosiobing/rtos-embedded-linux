# P02. The claim, and the service axis

Status on Tuesday 6 October 2026: **the C cascade is written and green**, in WSL on the
demo laptop, under `-Werror -Wconversion` with the address and undefined-behaviour
sanitisers. All seven of chapter 02's acceptance criteria pass, and the twenty-two cases
reach all seven claim codes, all eight arms, all seven service states and all seven
service reasons.

The design page is [docs/DESIGN.md](docs/DESIGN.md) and it was committed **before any
code**, which is chapter 02's first requirement and the reason the git history is the
evidence for it rather than this sentence.

The written design is [chapter 02](../../chapters/02-the-claim.md), which is complete.
What is absent is the other two languages, the cross-check and the kernels.

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
| [docs/figures](docs/figures) | the chapter's five figures, as rendered SVG |
| the other languages, the cross-check, the kernels | **not written** |

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

## What is planned, in the order P01 established

That order is not a preference. It is what made P01's claims checkable, and each step
exists to catch something the step before it cannot.

1. ~~The C cascade and its host test.~~ **Done and green.** The invariants are checked
   after every case rather than at the end, because a broken invariant can be transient
   and still wrong.
2. **The same cascade in C++17, C++23 and C++2c**, where the later two should be able to
   prove the arm order and the code count at compile time rather than by a test, as
   P01's `consteval` proofs do for its table.
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
