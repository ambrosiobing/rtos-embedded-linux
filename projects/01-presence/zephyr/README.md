# The presence table under Zephyr

**Status: written, not built.** No compiler has seen this directory, and the reason is
recorded rather than glossed: there was no Zephyr workspace, no `west` and no SDK in
WSL on the demo laptop when this was written, checked on Tuesday 6 October 2026. The
install is the step before the first build, and that build is the first test rather
than CI.

This is the second adapter. [`../freertos/`](../freertos/) was the first and is green,
and writing a second one is what showed which parts of the first were general.

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
| Where a timer expiry runs | in interrupt context, so it may only post with `K_NO_WAIT` | in the timer service task, which is less constrained but widens the cancel window |
| Static allocation | `K_MSGQ_DEFINE` and `K_THREAD_STACK_DEFINE` are static by construction | `configSUPPORT_DYNAMIC_ALLOCATION` at 0, so an allocating create fails to link |
| Priority order | downwards: a smaller number preempts | upwards: a larger number preempts |
| What `main` must do | nothing; it is already a thread with the scheduler running | hand the kernel memory for its idle and timer tasks, create a thread, start the scheduler |

The last row is the one with a consequence. Zephyr's `main.c` here is forty lines and
FreeRTOS's is a hundred, and nearly all of that difference is the memory the kernel
needs for its own two tasks when it cannot allocate.

**What does not differ at all** is the part that matters: the twenty-nine rows, the
three requirements in the contract, and the four phases. Neither kernel asked for a
change to any of them.

## Building it

`native_sim` builds with the **host compiler**, not the Zephyr SDK, so this needs a
Zephyr tree and `west` and nothing else. That is why the install is a tree rather than
a three gigabyte toolchain.

    source ~/zephyrproject/.venv/bin/activate
    west build -b native_sim -p always projects/01-presence/zephyr
    ./build/zephyr/zephyr.exe

## What will not be known even when it is green

The same thing the FreeRTOS page says, for the same reason. This is a simulation on a
general-purpose kernel: any latency measured here is a measurement of the host. What a
green run shows is that the table behaves identically when its events arrive through a
real queue and its releases come from a real timer, under a second kernel that
arranges both differently. The numbers that would mean something come from the board,
and no board has run any of this.
