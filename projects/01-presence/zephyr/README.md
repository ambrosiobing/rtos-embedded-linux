# The presence table under Zephyr

**Status: builds and runs on `native_sim` in WSL on the demo laptop, Tuesday 6 October
2026.** Three of the four phases passed on the first run and the fourth found a
difference between the two kernels rather than a defect; see below.

This is the second adapter. [`../freertos/`](../freertos/) was the first and is green,
and writing a second one is what showed which parts of the first were general.

### What the first build corrected

**Zephyr wants a toolchain variant even for `native_sim`.** With none set it looks for
the Zephyr SDK and stops when it is absent, which is not the same as needing the SDK's
compiler: `native_sim` compiles with the host gcc. `ZEPHYR_TOOLCHAIN_VARIANT=host` is
the whole of the fix and `CMakeLists.txt` now sets it when nothing else has, so the
documented build line is the build line. **No SDK is installed on this machine.**

**`CONFIG_STDOUT_CONSOLE` was refused by Kconfig** and has been removed from
`prj.conf`. It depends on not having a native library with an external libc, which is
exactly what `native_sim` is: the simulation uses the host's own libc and its stdout.
Setting it warned and changed nothing, and an option that is silently overridden is
worse than an absent one, because a reader believes it.

## What the second implementation changed about the first

An interface with one implementation is a guess about what is general. The FreeRTOS
adapter's header took a `long *higher_priority_task_woken` on the interrupt post,
because that is how FreeRTOS asks a caller to yield. Zephyr's `k_msgq_put` is the same
call from a thread or an interrupt and needs nothing of the sort.

So that parameter was never part of the contract. It was one kernel's calling
convention written into a header that three kernels have to satisfy, and it is gone:
each adapter now yields on its own behalf. The contract moved to
[`../adapter/presence_adapter.h`](../adapter/presence_adapter.h), where both
implementations include it, and the four test phases moved to
[`../adapter/phases.c`](../adapter/phases.c), which includes no kernel header at all.

That last move is what makes the comparison worth anything. The claim is not that each
adapter passes a test of its own. It is that **one test passes against each kernel**,
and the two things it needs that no contract can hide, a sleep and a way to outrank the
dispatch thread, are two named functions each adapter supplies.

## What is genuinely different, and what only looks different

| | Zephyr | FreeRTOS |
|---|---|---|
| The clock | `k_uptime_get_32` returns milliseconds | a tick count multiplied by `portTICK_PERIOD_MS`, in 64 bits to avoid an overflow nobody would see for weeks |
| Posting from an interrupt | the same call as from a thread, no yield flag | `xQueueSendToBackFromISR` plus `portYIELD_FROM_ISR` |
| A full queue with a receiver waiting | **absorbs one more than its depth.** `k_msgq_put` hands the message straight to a thread already blocked in `k_msgq_get`, bypassing the buffer | copies into the queue storage first, then unblocks the receiver, so the depth is the depth |
| Where a timer expiry runs | in interrupt context, so it may only post with `K_NO_WAIT` | in the timer service task, which is less constrained but widens the cancel window |
| Static allocation | `K_MSGQ_DEFINE` and `K_THREAD_STACK_DEFINE` are static by construction | `configSUPPORT_DYNAMIC_ALLOCATION` at 0, so an allocating create fails to link |
| Priority order | downwards: a smaller number preempts | upwards: a larger number preempts |
| What `main` must do | nothing; it is already a thread with the scheduler running | hand the kernel memory for its idle and timer tasks, create a thread, start the scheduler |

The last row is the one with a consequence. Zephyr's `main.c` here is forty lines and
FreeRTOS's is a hundred, and nearly all of that difference is the memory the kernel
needs for its own two tasks when it cannot allocate.

The second of those was found by the shared test failing, and it is the best argument
for having written the test that way. Phase 3 asserted that posting two past the end
of the queue is refused **twice**, which is true of FreeRTOS and false here: with the
dispatch thread pending, Zephyr handed one message over directly and only one post was
refused. The number was never the claim. What the phase exists to show is that a queue
which cannot take an event refuses it **and counts the loss**, and that is now what it
asserts, with the actual number printed so the log records which kernel did what. One
implementation cannot tell you which of your assertions are about the design and which
are about one kernel.

**What does not differ at all** is the part that matters: the twenty-nine rows, the
three requirements in the contract, and the four phases. Neither kernel asked for a
change to any of them.

## Building it

`native_sim` builds with the **host compiler**, not the Zephyr SDK, so this needs a
Zephyr tree and nothing else. That is why the install is a tree rather than a three
gigabyte toolchain.

    source ~/zephyrproject/.venv/bin/activate
    cmake -B build-zephyr -GNinja -DBOARD=native_sim -S projects/01-presence/zephyr
    ninja -C build-zephyr
    ./build-zephyr/zephyr/zephyr.exe

**Not `west build`, and the reason is worth knowing.** That is an extension command
which west discovers through its workspace manifest, so it exists only inside
`~/zephyrproject`; run from this repository it reports `unknown command "build"`.
`west zephyr-export` registers Zephyr's CMake package in `~/.cmake/packages/Zephyr`,
and `find_package(Zephyr)` finds it from anywhere, so the build needs neither a
workspace nor west once the tree is installed. That also makes a CI job simpler later.

### The version this is built against

    v4.5.0-rc1-170-g8f62a4ab82b

Recorded rather than pinned, and the difference from the FreeRTOS adapter is real.
That one clones `V11.1.0`, a release tag, so its Makefile names the version and the
fetch step prints what it got. `west init` with no revision takes the manifest's main
branch, and what arrived on Tuesday 6 October 2026 was a hundred and seventy commits
past `v4.5.0-rc1`, which is not a release and has no tag to pin. So this page records
the commit the claim was made against, which is the most that can honestly be said
until a release is cut.

## What will not be known even when it is green

The same thing the FreeRTOS page says, for the same reason. This is a simulation on a
general-purpose kernel: any latency measured here is a measurement of the host. What a
green run shows is that the table behaves identically when its events arrive through a
real queue and its releases come from a real timer, under a second kernel that
arranges both differently. The numbers that would mean something come from the board,
and no board has run any of this.
