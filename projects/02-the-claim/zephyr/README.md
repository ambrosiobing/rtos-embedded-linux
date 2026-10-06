# The claim cascade under Zephyr

**Status: green on `native_sim` in WSL on the demo laptop and a CI job, Tuesday 6 October
2026.** All four phases passed on the first run, which is worth saying plainly because
[P01's second adapter](../../01-presence/zephyr/) did not: its first run found a FreeRTOS
assumption inside the shared test. This one found nothing, and that is the contract from P01
earning its keep rather than this adapter being written more carefully.

This is the second adapter. [`../freertos/`](../freertos/) was the first.

## What is genuinely different, and what only looks different

| | Zephyr | FreeRTOS |
|---|---|---|
| The clock | `k_uptime_get_32` returns milliseconds | a tick count multiplied by `portTICK_PERIOD_MS` |
| Posting from an interrupt | the same call as from a thread, no yield flag | `xQueueSendToBackFromISR` plus `portYIELD_FROM_ISR` |
| A full queue with a receiver waiting | **absorbs one more than its depth**, so one post past the end is refused | the depth is the depth, so two are refused |
| Where the grace expiry runs | in interrupt context, so it may only post with `K_NO_WAIT` | in the timer service task, which is less constrained |
| Static allocation | `K_MSGQ_DEFINE` and `K_THREAD_STACK_DEFINE` are static by construction | `configSUPPORT_DYNAMIC_ALLOCATION` at 0, so an allocating create fails to link |
| Priority order | downwards: a smaller number preempts | upwards: a larger number preempts |
| What `main` must do | nothing; it is already a thread with the scheduler running | hand the kernel memory for its idle and timer tasks, create a thread, start the scheduler |

The priority row is the one most likely to be got backwards when porting between the two, so
the adapters' priority constants are written to be read beside each other rather than each in
its own idiom.

**The timer row did not force a difference, and that is a choice.** Zephyr's expiry runs in
interrupt context and may only post with `K_NO_WAIT`; FreeRTOS's runs in a task and could
have written the inputs directly under a critical section. Both post an event instead, which
keeps the dispatcher the only writer, so neither adapter needs a lock and the shared phases
never have to know which kernel they are on.

## What the run prints

    1. the chapter's sequence and both axes, through a real queue and thread
    2. a no-show caused by the grace timer, not by an injected input
    3. a full queue loses an event, and says so
       depth 16, posted 18, refused 1
    4. the claim still changes and the spool still holds it, with no link
       offered 609, retained 512 of 512, discarded 97

**Phase 3 is the only line where the two kernels disagree**, and the disagreement is the
finding: `k_msgq_put` hands a message straight to a thread already blocked in `k_msgq_get`,
bypassing the buffer, so with the dispatcher pending the queue absorbs one more than its
depth. FreeRTOS copies into the queue storage first and then unblocks the receiver. Neither
is wrong. The phase asserts that a refusal happens and is counted, and prints the number.

Phase 4 agrees exactly, which is the stronger half: the same 609 events, the same 512
retained, the same 97 discarded, from one source file compiled for two kernels.

## Building it

`native_sim` builds with the **host compiler**, not the Zephyr SDK, so this needs a Zephyr
tree and no toolchain install at all. The one thing Zephyr insists on, a named toolchain
variant, is set in `CMakeLists.txt`, so the three lines below are the whole of it.

    source ~/zephyrproject/.venv/bin/activate
    cmake -B build-claim -GNinja -DBOARD=native_sim -S projects/02-the-claim/zephyr
    ninja -C build-claim
    ./build-claim/zephyr/zephyr.exe

**Not `west build`**, which is an extension command west discovers through its workspace
manifest and so exists only inside `~/zephyrproject`. `west zephyr-export` registers Zephyr's
CMake package once, and `find_package(Zephyr)` then finds it from anywhere. The full argument
is on [P01's Zephyr page](../../01-presence/zephyr/), which is where it was worked out.

### The version this is built against

    v4.5.0-rc1-170-g8f62a4ab82b

Recorded rather than pinned, for the reason P01's page gives: `west init` with no revision
takes the manifest's main branch, and what arrived was a hundred and seventy commits past a
release candidate, which is not a release and has no tag to pin. The CI job pins
`v4.5.0-rc1` instead, so the adapter is built at two points of the tree and the page says so
rather than implying they are one.

## What will not be known even when it is green

The same thing [`../freertos/`](../freertos/) says, for the same reason. This is a simulation
on a general-purpose kernel: any latency measured here is a measurement of the host. What a
green run shows is that the cascade reaches the same decisions under a second kernel that
arranges its queue, its timer and its priorities differently. The numbers that would mean
something come from the board, and no board has run any of this.
