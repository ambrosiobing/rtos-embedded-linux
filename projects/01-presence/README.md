# P01. Presence: free, occupied, held, fault

Status: the table and its host test are written. **Nothing has been compiled on
this machine and nothing has run on a board.** The compilers are in CI and in WSL;
the laptop this was written on runs none.

The written design is [chapter 01](../../chapters/01-presence.md). The design page
for the code is [docs/DESIGN.md](docs/DESIGN.md), and it was written before the
code, which is the chapter's first requirement.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, and nothing else |
| Board | NUCLEO-H7A3ZI-Q alone. No shield, no instrument, no radio |
| Peripherals | The three on-board indicators as a state display, the virtual console for the event log, the user button as a forced event, one kernel timer |
| Difficulty | 3 of 5 |
| Effort | 3 evenings of about four hours |

## What exists

| Part | State |
|---|---|
| [docs/DESIGN.md](docs/DESIGN.md) | the 28 rows, the invariant, the defaults, and two Mermaid diagrams generated from the same rows |
| [c/presence.h](c/presence.h) | four states, six events, the context and the invariant check |
| [c/presence.c](c/presence.c) | the 28 rows and a dispatcher of a dozen lines. No hardware in it |
| [c/test_presence.c](c/test_presence.c) | the host test: every row reachable, the order traps, and the invariant after every dispatch |
| [cpp/presence.hpp](cpp/presence.hpp) | the same 28 rows in C++17, the baseline that compiles unchanged under all three versions |
| [cpp/presence23.hpp](cpp/presence23.hpp) | `std::expected` as the dispatch return, and `consteval` proofs that the table is total and its guards are ordered |
| [cpp/presence26.hpp](cpp/presence26.hpp) | `std::inplace_vector` for the sixteen-deep queue, and the feature report |
| [cpp/test_presence.cpp](cpp/test_presence.cpp) | the C test's sequences in C++, plus the report of which version-specific additions the compiler provided |
| [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md) | what each version actually changes for this table, filled from the CI log |
| Rust | not started |
| the kernel adapters | not started |

## What the table is, and why it is the specification

Four states and six events give 24 pairs. Four of those pairs carry a second
guarded row, so the table has **28 rows and is total**: every state and event
combination has a row. A dispatcher that found no row returns an error rather than
dropping the event, because a dropped event is how a release goes missing.

**The invariant is that a release cannot be lost.** A room that forgets to release
is worse than a room with no sensor, because a closed door and a lit indicator
look identical whether the room is in use or whether the firmware stopped paying
attention. `presence_check_invariants` is that invariant written down, and the
host test calls it after **every single dispatch** rather than at the end, because
a lost release can be transient and still wrong.

## Two findings from writing it

**The table was not total in its first draft.** `FREE` and `OCCUPIED` had no
`TIMEOUT` row, and a stale hold timer genuinely arrives in both: row 15 cancels a
hold when a reading returns, by which time the kernel may already have queued the
expiry. Those two rows, 4 and 11, do nothing on purpose, and without them a
legitimate race would have returned `ERR_NO_ROW`. The totality claim in the header
is what caught it, which is the argument for writing the claim down.

**The row index is asserted, not only the state.** Rows 0 and 1 share a from-state
and an event and differ only by their guard. Swapping them delays every arrival by
one reading and leaves every final state identical, so a test of outcomes alone
would pass against it. That is why `presence_dispatch` records `last_row` and why
the test checks it.

## How it is built, and where

Nothing in this project needs the board. That is unusual in this volume and is the
point: the acceptance criterion here is a test result rather than a measurement.

`code.yml` compiles `presence.c` and `test_presence.c` under **C11, C17 and C23**,
with gcc and with clang, at `-Werror` with `-Wconversion` and the sanitisers, and
runs the row-coverage test under each. The same two source files under all six
combinations, with no per-version source and no conditional compilation: the claim
this project makes is that the table is expressible without reaching for anything
a later version added, and a version that fails is a finding about that claim
rather than a build problem to patch around.

In WSL on the demo laptop, the same thing by hand:

    gcc -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -Wconversion \
        -o build-host/test_presence \
        projects/01-presence/c/presence.c projects/01-presence/c/test_presence.c
    ./build-host/test_presence

## What is not here yet

- **C++ and Rust.** The same table under C++17, C++23 and C++26, and under Rust's
  editions, with `docs/LANGUAGE_IDIOMS.md` reporting what each version actually
  buys for this table rather than what its release notes advertise.
- **The kernel adapters.** A thread, a queue, a timer and three lamps per kernel,
  none of which is allowed to make a decision. `docs/RTOS_VARIANTS.md` will carry
  the mapping and name every place the design had to change rather than be
  renamed.
- **Anything on hardware.** No board has run this.
