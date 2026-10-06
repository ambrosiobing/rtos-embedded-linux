# The claim cascade under FreeRTOS

**Status: green in WSL on the demo laptop and a CI job, Tuesday 6 October 2026.** It builds,
it links, no allocator is in the binary, and all four phases pass. The cascade that
[`../c/test_claim.c`](../c/test_claim.c) exercises by calling `claim_decide` directly now
runs with a real queue, a real dispatch task and a real software timer, and reaches the same
decisions.

## What this adapter has to do that the pure cascade does not

The cascade is a function of a set of inputs. It has no queue, no clock and no memory of
what it decided last. So the adapter holds the inputs, an event changes one of them, and the
decision is recomputed from the whole set rather than advanced from the previous one.

That is the chapter's own sentence arriving at a kernel: **a claim is a decision, not a
state.** [P01's adapter](../../01-presence/freertos/) forwards events into a machine that
advances; this one keeps a picture of the world and asks a question about it.

## The grace timer, which is where the kernel earns its place

When the cascade says `GRACE` the adapter arms a one-shot timer. When it says anything else
the adapter stops it. If the timer fires, it posts `GRACE_EXPIRED`, the dispatcher sets
`past_grace`, and the next decision is a no-show **caused by a clock**. Every other no-show
in this project is one a test wrote.

Two decisions in that are policy rather than plumbing.

**Opening a window clears `past_grace`.** A new booking starts a new grace period and must
not inherit the last one's expiry, or the second booking of the day would no-show the moment
it opened.

**The timer callback posts an event rather than writing the inputs.** That keeps the
dispatcher the only writer, so neither adapter needs a lock anywhere. FreeRTOS runs its timer
callbacks in the timer service task, which is a task and could have written safely with a
critical section; Zephyr runs its expiry in interrupt context and could not. Writing both the
same way means the shared phases do not have to know which kernel they are on.

## What the run prints

    1. the chapter's sequence and both axes, through a real queue and thread
    2. a no-show caused by the grace timer, not by an injected input
    3. a full queue loses an event, and says so
       depth 16, posted 18, refused 2
    4. the claim still changes and the spool still holds it, with no link
       offered 609, retained 512 of 512, discarded 97

**Phase 3's number is the kernel's and not the design's.** FreeRTOS refuses two because it
copies into the queue storage and then unblocks the receiver. Zephyr refuses one. The phase
asserts that a refusal happens and is counted, and prints how many, which is the correction
P01 made after asserting two and meeting a kernel that does one.

## Building it

    make fetch     clone the kernel at V11.1.0, if it is not already there
    make run       build and run, which is what CI does

The kernel is cloned at a pinned tag rather than vendored, which keeps ten thousand lines
nobody here wrote out of a published repository and makes the version part of the build. If
[P01's](../../01-presence/freertos/) clone is already at `~/src/FreeRTOS-Kernel`, `make
fetch` reports it and leaves it alone.

Three sets of compiler flags, which P01 worked out at the cost of a build that failed on the
dialect: the kernel and port at `gnu11` and not held to this project's warnings, the adapter
at `gnu11` with `_GNU_SOURCE` and the full wall, and [`../c/claim.c`](../c/claim.c) at
**strict C11**, because the claim this volume makes about the policy is that it needs nothing
beyond C11.

**The target lists its sources, which it did not until Tuesday 6 October 2026.** Before that
the rule had only an order-only prerequisite on the build directory, so once the binary
existed `make run` ran it whatever had changed. A pull that altered the shared phases was
followed by a run of the binary from before the pull, and it printed a pass. What caught it
was Zephyr building the same file from scratch and printing different numbers for the same
commit.

## What will not be known even when it is green

This is a host build on a general-purpose kernel: any latency measured here is a measurement
of the host. What a green run shows is that the cascade reaches the same decisions when its
inputs arrive through a real queue and its grace period is ended by a real timer. The numbers
that would mean something come from the board, and no board has run any of this.
