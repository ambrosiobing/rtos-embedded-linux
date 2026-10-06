# P01. Presence: free, occupied, held, fault

Status on Tuesday 6 October 2026, with the twenty-ninth row in: the table and its
host tests pass in WSL on the demo laptop. The C under C11, the C++ under C++17, and
the Rust under all three editions, 33 tests in total. **CI's last three runs are red
and the cause is fixed but not yet re-run**: a scripted edit split a string literal
in the C++ test, which took the six C++ jobs down. The C++23 and C++26 builds have
not been run anywhere since the table changed. **Nothing has run on a board, and
nothing is compiled on the laptop this was written on**, which runs no compiler for
it: the toolchains are in CI and in WSL on the demo laptop.

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
| [docs/DESIGN.md](docs/DESIGN.md) | the 29 rows, the invariant, the defaults, and two Mermaid diagrams generated from the same rows |
| [c/presence.h](c/presence.h) | four states, six events, the context and the invariant check |
| [c/presence.c](c/presence.c) | the 29 rows and a dispatcher of a dozen lines. No hardware in it |
| [c/test_presence.c](c/test_presence.c) | the host test: every row reachable, the order traps, and the invariant after every dispatch |
| [cpp/presence.hpp](cpp/presence.hpp) | the same 29 rows in C++17, the baseline that compiles unchanged under all three versions |
| [cpp/presence23.hpp](cpp/presence23.hpp) | `std::expected` as the dispatch return, and `consteval` proofs that the table is total and its guards are ordered |
| [cpp/presence26.hpp](cpp/presence26.hpp) | `std::inplace_vector` for the sixteen-deep queue, and the feature report |
| [cpp/test_presence.cpp](cpp/test_presence.cpp) | the C test's sequences in C++, plus the report of which version-specific additions the compiler provided |
| [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md) | what each version actually changes for this table, filled from the CI log |
| [rust/presence_core.rs](rust/presence_core.rs) | the same 29 rows, `no_std`, no `unsafe`, one source compiled under three editions |
| [rust/tests_core.rs](rust/tests_core.rs) | the same cases, plus the exhaustive `match` held against the table over 192 combinations |
| `rust/e2018`, `rust/e2021`, `rust/e2024` | three crates differing only in their edition line |
| [docs/RTOS_VARIANTS.md](docs/RTOS_VARIANTS.md) | the contract between the table and a kernel, the mapping for Zephyr, FreeRTOS and QNX, and the argument for a twenty-ninth row |
| [scripts/crosscheck_table.py](../../scripts/crosscheck_table.py) | the three tables compared row for row, in CI, so "the same table" is enforced rather than repeated |
| [freertos/](freertos/) | the first kernel adapter: one queue, one dispatch task, two timers, static allocation only. **Written, not yet built** |
| the Zephyr and QNX adapters | not written. Zephyr has no workspace in WSL to build against, and QNX has no licence or target here at all |

## What the table is, and why it is the specification

Four states and six events give 24 pairs. Four of those pairs carry more than
one row, told apart by their guards, which is five extra rows, so the table has
**29 rows and is total**: every state and event combination has a row. A dispatcher that found no row returns an error rather than
dropping the event, because a dropped event is how a release goes missing.

**The invariant is that a release cannot be lost.** A room that forgets to release
is worse than a room with no sensor, because a closed door and a lit indicator
look identical whether the room is in use or whether the firmware stopped paying
attention. `presence_check_invariants` is that invariant written down, and the
host test calls it after **every single dispatch** rather than at the end, because
a lost release can be transient and still wrong.

## Three findings from writing it

**The table was not total in its first draft.** `FREE` and `OCCUPIED` had no
`TIMEOUT` row, and a stale hold timer genuinely arrives in both: row 15 cancels a
hold when a reading returns, by which time the kernel may already have queued the
expiry. Those two rows, 4 and 11, do nothing on purpose, and without them a
legitimate race would have returned `ERR_NO_ROW`. The totality claim in the header
is what caught it, which is the argument for writing the claim down.

**Row 17 needed a guard, and only a kernel's documentation said so.** The release
row was unguarded, which is safe only while events are dispatched in the order they
were posted. A QNX channel delivers pulses in priority order, where an expiry from a
cancelled hold can arrive after a newer hold has started and release it up to thirty
seconds early. An early release loses a presence as surely as a missing one, and it
is harder to see: every state along the way is legal and the invariant holds at every
step. Row 17 is now guarded by the hold having actually expired and row 18 absorbs
the rest, which is the only change these rows have had since the C was written. The
argument is in [docs/RTOS_VARIANTS.md](docs/RTOS_VARIANTS.md) and was written first.

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

- **The C++23 and C++26 builds, and a green CI run.** The twenty-ninth row has been
  through C11, C++17 and all three Rust editions in WSL. What it has not been
  through is the two later C++ standards, where `presence23.hpp` proves the guard
  order at compile time with a `consteval` check and `presence26.hpp` asserts the
  row count: both of those read the table and neither has seen 29 rows. Nor has
  CI, whose last three runs are red for a cause since fixed. Six refusals have come
  before this point and all six are recorded in
  [docs/LANGUAGE_IDIOMS.md](docs/LANGUAGE_IDIOMS.md#what-the-compiler-rejected-in-wsl-on-monday-5-october-2026)
  rather than quietly fixed.
- **The bare-metal link.** Whether the library is really `no_std` is shown by none
  of the above, because the test configuration pulls in std for the harness. Only
  CI checks it, by building the library alone for `thumbv7em-none-eabihf`.
- **A build of the FreeRTOS adapter.** [freertos/](freertos/) is written and no
  compiler has seen it. Its four phases include the two things nothing else here
  can show: a release produced by a real kernel timer rather than by an injected
  event, and a full queue counted rather than dropped in silence.
- **The Zephyr and QNX adapters.** The mapping and the three requirements any
  adapter must satisfy are in [docs/RTOS_VARIANTS.md](docs/RTOS_VARIANTS.md).
  Zephyr has no workspace or SDK in WSL on the demo laptop, checked on Tuesday
  6 October 2026, so it has no local loop; QNX has no licence and no target here
  at all and will be written without ever being built.
- **Anything on hardware.** No board has run this.
