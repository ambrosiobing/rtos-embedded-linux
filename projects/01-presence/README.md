# P01. Presence: free, occupied, held, fault

Status: the table and its host tests are written and they pass. The C and the C++
in CI, under six compiler and standard combinations; the Rust in WSL, under three
editions. **Nothing has run on a board, and nothing is compiled on the laptop this
was written on**, which runs no compiler for it: the toolchains are in CI and in
WSL on the demo laptop.

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
| [rust/presence_core.rs](rust/presence_core.rs) | the same 28 rows, `no_std`, no `unsafe`, one source compiled under three editions |
| [rust/tests_core.rs](rust/tests_core.rs) | the same cases, plus the exhaustive `match` held against the table over 96 combinations |
| `rust/e2018`, `rust/e2021`, `rust/e2024` | three crates differing only in their edition line |
| [docs/RTOS_VARIANTS.md](docs/RTOS_VARIANTS.md) | the contract between the table and a kernel, the mapping for Zephyr, FreeRTOS and QNX, and the argument for a twenty-ninth row |
| the kernel adapters themselves | not written. The design page above comes first, which is this chapter's own rule |

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

- **The bare-metal link.** Formatting, clippy at `-D warnings` and all 27 tests
  pass in WSL, nine under each edition, the 96-combination cross-check between the
  exhaustive `match` and the array included. What that does not show is that the
  library is `no_std`, because the test configuration pulls in std for the
  harness. Only CI checks it, by building the library alone for
  `thumbv7em-none-eabihf`. Five refusals came before that green run, two of them
  on edition grounds, and all five are recorded in
  [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md#what-the-compiler-rejected-in-wsl-on-monday-5-october-2026)
  rather than quietly fixed.
- **A twenty-ninth row.** Row 17, the only release, is unguarded, and
  [docs/RTOS_VARIANTS.md](docs/RTOS_VARIANTS.md#the-defect-requirement-1-was-hiding)
  shows that this is safe only while events are dispatched in the order they were
  posted. QNX delivers pulses on a channel in priority order, where a stale hold
  expiry can arrive after a newer hold has started and release it early, which
  loses a presence as surely as never releasing does. The fix is a guard in the
  table rather than a rule in an adapter, and it touches all three languages, both
  cross-checks and the memory figures. The argument is written; the row is not.
- **The kernel adapters.** A thread, a queue, a timer and three lamps per kernel,
  none of which is allowed to make a decision. The mapping and the three
  requirements an adapter has to satisfy are in
  [docs/RTOS_VARIANTS.md](docs/RTOS_VARIANTS.md). FreeRTOS and Zephyr can both run
  theirs on a host and in CI; QNX cannot be built on this bench at all, and that
  adapter will say so in its own header.
- **Anything on hardware.** No board has run this.
