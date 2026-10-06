# The presence table under FreeRTOS

**Status: green in WSL on the demo laptop and green in CI, Tuesday 6 October 2026.** It builds,
it links, no allocator is in the binary, and all four phases pass. The table that the
C, the C++ and the Rust suites exercise by calling `presence_dispatch` directly now
runs with a real queue, a real dispatch task and a real software timer, and takes the
same rows. `code.yml` runs it on every push, so it is a gate rather than something
that happened once.

Three builds were needed and each failure is recorded rather than quietly fixed,
because what they corrected was not what had been predicted.

**The first build.** The uncertainty flagged in advance was the source list: which
files the POSIX port needs beside `port.c`. **That part was right.** `port.c` and
`utils/wait_for_event.c` are the two, and both reached their own function bodies. What
was wrong was the dialect. `-std=c11` is strict ISO C, under which glibc hides every
POSIX declaration, so a port whose entire job is `pthread_sigmask`, `sigaction`,
`clock_gettime`, `usleep`, `CLOCK_MONOTONIC` and `PTHREAD_STACK_MIN` found none of
them declared.

The fix is `-std=gnu11` with `_GNU_SOURCE` for the kernel and for the adapter, and the
Makefile now carries three sets of flags instead of two. `../c/presence.c` keeps
strict `-std=c11`, deliberately: the claim this volume makes about the table is that
it needs nothing beyond C11, and compiling it as `gnu11` here to save a line of
Makefile would have retired that claim to spare an inconvenience.

**The second build** reached the adapter and found two things there. The memory hooks
the kernel calls when dynamic allocation is off take `configSTACK_DEPTH_TYPE` as their
third parameter, not `uint32_t`, which on this host is a real difference in width. And
`post_plain` had been written and never called, which `-Werror` refuses. The
`-Wconversion` correction predicted for this build did not happen: `pdMS_TO_TICKS`
casts to `TickType_t` throughout and the constants provably fit, so the flag had
nothing to say.

**The third build was green.** What all three have in common is worth more than the
fixes: not one of them was in the table, the adapter or the design. Two were about
which declarations a dialect makes visible and one was a function signature in a
kernel's own API.

**Then the Zephyr adapter changed this one.** The contract and the four phases moved
to [`../adapter/`](../adapter/), and the interrupt post lost its
`long *higher_priority_task_woken` parameter, because that was FreeRTOS's way of
asking a caller to yield and not something a second kernel needed. This adapter yields
on its own behalf now. An interface with one implementation is a guess about what is
general; the second one is what tests the guess.

## Why FreeRTOS first of the three

Of the three kernels in [docs/RTOS_VARIANTS.md](../docs/RTOS_VARIANTS.md), this is
the only one whose adapter can run **both** on a host and in CI with nothing but gcc,
because the kernel ships an official POSIX port. Zephyr can do the same through
`native_sim`, but there is no Zephyr workspace or SDK in WSL on the demo laptop,
checked on Tuesday 6 October 2026, so it has no local loop yet. QNX has no licence and
no target here at all and will be written without ever being built.

So this is the one place in this project where "the same table under a real kernel"
can be a test result rather than a reading of the source.

## What is here

| | |
|---|---|
| [../adapter/presence_adapter.h](../adapter/presence_adapter.h) | the contract, shared with every other adapter. It lived here until the Zephyr adapter showed which part of it was FreeRTOS's calling convention rather than anyone's requirement |
| [../adapter/phases.c](../adapter/phases.c) | the four test phases, shared, and containing no kernel header, so one test runs against every kernel |
| [presence_adapter.c](presence_adapter.c) | one queue, one dispatch task, two software timers, and nothing that decides |
| [FreeRTOSConfig.h](FreeRTOSConfig.h) | static allocation only, with dynamic allocation switched off so that an accidental `xQueueCreate` fails to link |
| [host_main.c](host_main.c) | what only FreeRTOS needs: memory for the kernel's own idle and timer tasks, a place for a failed assertion, and a thread to run the phases on |
| [Makefile](Makefile) | fetches the kernel at a pinned tag, builds, and refuses a binary containing an allocator |

## What the adapter may not do, and how that is visible

The rule is negative, which is the useful form: **no comparison against a range, no
count of readings, no notion of how long a hold lasts, no opinion about what an event
means.** A threshold found in this directory means this directory is wrong, not the
table. Every number lives in `presence_settings_t` and every decision is a row.

The adapter reads exactly one field of the machine, `hold_running`, and only to
mirror it onto the kernel timer: false to true arms it for `settings.hold_ms`, true to
false cancels it. It reads that after `presence_dispatch` returns and never writes it.

## The three things the table needs back

1. **One queue, one consumer, first in first out.** `xQueueSendToBack` and one task.
   `xQueueSendToFront` and `xQueueOverwrite` are never called, and that is not style:
   either would remove the ordering guarantee that makes a stale expiry harmless.
2. **`at_ms` stamped when an event is posted.** Every post reads the tick count at the
   moment of posting, and a timer callback reads it when it fires. An adapter that
   stamped at the front of the dispatch loop would make every stale event look fresh
   and defeat the guard on row 17.
3. **A cancelled hold may still deliver its expiry.** `xTimerStop` posts a command to
   the timer service task rather than acting at once, so this window is wider here
   than under Zephyr. Rows 4, 11 and 18 absorb it. The adapter drops nothing.

## What the four phases are for

**1. The scripted sequence through the queue.** The same events as
`../c/test_presence.c`, posted into a real queue and taken by a real task, asserting
the same row sequence. The dispatch task runs above the test task, so each post is
consumed before the next is made and the comparison is deterministic rather than a
race that usually passes.

**2. A release by the kernel timer.** The hold is shortened to 60 ms through a
SETTINGS event, which is the table's own way of changing it, and then nothing is
posted. If row 17 is reached it is because a software timer fired and the callback
posted a TIMEOUT. **Until this runs, every TIMEOUT in this project has been written by
a test**, and the one thing a timer adapter exists to do has never been done.

**3. A full queue.** The one failure this design cannot tolerate and cannot detect
afterwards: a lost event is a lost release. This phase raises its own priority above
the dispatcher so nothing drains, posts two events more than the queue holds, and
asserts that both refusals were counted. An adapter that dropped silently would pass
every other phase here.

**4. The heartbeat**, started last so that a periodic TICK does not interleave with
the sequences above. That is also why `presence_adapter_start_tick` is separate from
`presence_adapter_init`.

## Running it

The kernel is not vendored into this repository. It is cloned at a pinned tag, which
keeps ten thousand lines nobody here wrote out of a published portfolio and makes the
version part of the build rather than of the history. The default clone path is
outside this tree on purpose.

    cd projects/01-presence/freertos
    make fetch
    make run

`make` also checks the binary for `pvPortMalloc` and `vPortFree` and fails if either
is present. With `configSUPPORT_DYNAMIC_ALLOCATION` at 0 they are not compiled at all,
so this is a check that the configuration is what it claims rather than a measurement
after the fact.

## What will not be known even when it is green

Everything about timing. This is a host build on a general-purpose kernel: the tick is
software, the tasks are pthreads, and any latency measured here is a measurement of
the host. What a green run shows is that the table behaves identically when its events
arrive through a real queue and its releases come from a real timer. The numbers that
would mean something come from the board, and no board has run any of this.
