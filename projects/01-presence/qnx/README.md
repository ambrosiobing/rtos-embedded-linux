# The presence table under QNX

**Status: written, and it will never be built here.** There is no QNX licence and no
QNX target on this bench, and there is not going to be one. Every API call in this
directory comes from the documentation and none of it from a compiler, which is a
weaker kind of evidence than anything else in this project. That is stated here, in
the source header, and in the project page, rather than left for a reader to work out.

[`../freertos/`](../freertos/) is green and runs in CI. [`../zephyr/`](../zephyr/) is
written against a toolchain that exists on the demo laptop. This is a design in C that
argues for itself.

## Why it is here at all

Two reasons, and the second is the one that earned its place.

**The honest first reason.** QNX appeared in **zero of the twenty-three** embedded
postings counted on Saturday 3 October 2026, which is why the runtime coverage note
parked it with a written revival condition. It is in this volume as a decision about
breadth in high rate contract work, not as a conclusion from those postings.

**The reason that justified it.** QNX is the kernel that does not fit, and the two
places it does not fit are the only interesting rows in the mapping table of
[`../docs/RTOS_VARIANTS.md`](../docs/RTOS_VARIANTS.md). **One of them changed the
table that every other implementation in this project uses.** Writing an adapter for a
kernel that fits teaches very little. This one produced the twenty-ninth row.

## The first misfit: a pulse carries four bytes and the event is twenty

`MsgSendPulse` takes an `int`. `presence_event_t` is a kind, a timestamp and a
twelve-byte union. So the design has to move rather than be renamed, and three ways
out were considered:

| | | |
|---|---|---|
| Send a message instead | `MsgSend` carries any payload | **rejected.** It blocks the sender until a server replies, so every reading becomes a rendezvous and the sensor thread gains a way to be blocked by the dispatcher. The dispatcher would stop being the only thing deciding order |
| A ring here, a pulse as a doorbell | the adapter owns the queue | **taken** |
| Shrink the event to four bytes | no ring needed | **rejected**, and it is the one that looks cheapest. A settings event carries eight bytes, so the six settings rows would stop being events and the table would stop being the whole specification |

The cost of the choice is stated rather than hidden: **the queue is this adapter's and
not the kernel's**, so requirement 1 of the contract, one queue in order, is this
file's to guarantee. Under Zephyr and FreeRTOS the kernel guarantees it and the
adapter only has to avoid the calls that would break it.

**A fourth option exists and the design page did not consider it.** POSIX message
queues, `mq_open` and `mq_send`, carry an arbitrary payload and are available on QNX,
which would remove the ring entirely. It is not taken, for two reasons rather than by
oversight: this chapter is about the native IPC model, where a pulse is the primitive
a reader came to see, and an `mq` on QNX is served by a resource manager that has to be
running, which turns a self-contained adapter into one with a deployment dependency.
If this were production rather than a comparison, that option would deserve the
measurement that would settle it.

## The second misfit: a channel delivers by priority, not in order

`MsgReceivePulse` returns the highest priority pulse waiting. A pulse from a high
priority sensor thread can therefore be received ahead of an older one from a lower
priority timer, which breaks requirement 1 outright.

**This is what guarded row 17.** An expiry belonging to a hold that was already
cancelled could be delivered after a *newer* hold had started, and an unguarded row 17
would have released it up to thirty seconds early. A release that happens early loses a
presence as surely as one that never happens, and it is harder to see, because every
state along the way is legal and the invariant holds at every step.

This adapter does two things about it, and both are deliberate:

1. **Every pulse is sent at one priority**, so the channel cannot reorder them.
2. **The table carries the guard anyway**, because a single priority is a property of
   this one file, and the table should not have to trust it.

The guard was added for this kernel. It costs the other two nothing.

## A third difference, smaller but real

An interrupt on QNX is handled by `InterruptAttachEvent`, which delivers an event to a
thread rather than running code in interrupt context. So
`presence_adapter_post_from_isr` is not called from an interrupt service routine here,
and the name survives only because the contract is shared. That matters because a true
interrupt service routine could not take the mutex this ring needs: owning the queue
means owning its concurrency, which is the cost of the first misfit showing up a second
time.

## Where QNX is the better fit

One place, and it is worth saying because the rest of this page is a catalogue of the
opposite. A QNX timer delivers a **sigevent**, and the sigevent here is a pulse. So a
timer expiry arrives through the same channel, at the same priority, as every other
event. There is no callback, no interrupt context and no second path into the machine.
Under FreeRTOS an expiry runs in the timer service task and under Zephyr in interrupt
context, and both have to post into the queue from there.

## What is not here

No build file. A Makefile for a toolchain nobody here has would be a guess dressed as
an instruction. The two adapters that can be built carry build files that have been
run, and this one carries the design and says so.
