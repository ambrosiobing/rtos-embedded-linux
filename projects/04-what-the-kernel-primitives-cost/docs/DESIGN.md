# P04 design: an instrument first, and the numbers after

Written on Thursday 8 October 2026, **before any code in this project exists**, which is the
requirement every project in this volume starts with and the reason the git history is the
evidence rather than this sentence. Every statement below about a cost or a measurement is a
requirement on work not yet done.

## The claim, and why it is worth a chapter

**Every later chapter in this volume states a duration at some point.** How long a dispatch
takes, how long a sensor read blocks, how long a radio is awake. A volume that states those
numbers without saying how they were obtained is a volume whose numbers cannot be checked, and
**a number that cannot be checked is worth less than no number at all, because it looks like
evidence.**

So this chapter does the measuring once and the rest of the book cites it. It prices the
kernel's own primitives against each other: a switch between threads, a message queue against
waiting on several objects at once, handing work to a queue against doing it in place, and a
mutex when a lower-priority thread holds it and a higher-priority one wants it.

## The boundary that shapes everything else

This bench has two instruments that can see time, and the gap between them is the subject.

| Instrument | Resolves | Therefore |
|---|---|---|
| the processor's own cycle counter | single cycles | every primitive in the table |
| the acquisition board on a Linux host | about ten microseconds on one channel | periods, and nothing shorter |

A context switch on this processor is far shorter than ten microseconds. **Using the
acquisition board to measure one would produce a number, and the number would be a property of
the instrument rather than of the software.** Knowing which instrument cannot answer a question
is as much the point of this chapter as the answers are, and one acceptance criterion exists
purely to enforce it.

## The decision that goes beyond the chapter, and what it costs

[Chapter 04](../../../chapters/04-what-the-kernel-primitives-cost.md) specifies one operating
system: several threads, a message queue, a work queue, a mutex. **This project will price the
same primitives under two kernels**, Zephyr and FreeRTOS, as
[P01](../../01-presence/) and [P02](../../02-the-claim/) already do for behaviour.

That is an extension rather than something the chapter asked for, and it is written here as a
decision rather than slipped in. The reasons:

- **A ratio between two primitives is more useful than a cost**, and a ratio that holds under
  two kernels is more useful still, because it is then a property of the pattern rather than of
  one implementation.
- The volume already carries both kernels, so the adapter shape is established and the cost is
  the harness rather than the learning.
- A reader choosing a primitive wants to know whether the choice survives a change of kernel.

**What it costs:** two harnesses to keep honest instead of one, and a real risk of comparing
two things that are not the same operation. A queue in one kernel is not automatically the
queue in the other. That risk is the reason for the primitive map below, and it is the same
risk [P02's adapter](../../02-the-claim/adapter/) was built to manage.

**What it does not license:** any claim that one kernel is faster than the other. The
comparison is of primitives against each other **within** a kernel, and then of those ratios
across kernels. A table reading "Zephyr 180 cycles, FreeRTOS 210 cycles" would be comparing two
configurations, two compilers' inlining decisions and two sets of defaults, and would say
almost nothing.

## The primitive map, which is written before either harness

Three implementations of one measurement are three chances to measure three different things,
and no test inside any of them can notice the drift, because each is right about itself. That
lesson is already in [P02](../../02-the-claim/docs/DESIGN.md) and it applies harder here,
because a timing number carries no sign of being about the wrong operation.

So each row names the operation **by what happens**, not by what either kernel calls it, and
the map from that to each kernel's name is written before the code.

| What happens | Why it is in the table |
|---|---|
| control passes from one ready thread to another of the same priority | the floor. Everything else is this plus something |
| a thread blocks on one object, another signals it, control returns | the common case in every later chapter |
| a thread blocks until any of several objects is ready | the alternative to one object, and the reason to know its price |
| work is handed to a queue rather than done in place | the usual answer to a long interrupt, priced against not doing it |
| a thread blocks on a mutex a lower-priority thread holds, with a medium-priority thread runnable | the only row where the answer depends on a protocol rather than on a count |

The last row is the one worth the chapter on its own, and it is the one with a built-in
refutation: **with priority inheritance on, the high-priority thread's wait should not include
the medium thread's work, and with it off it should.** Two configurations, one program, and a
difference that is predicted before it is measured.

## The seven criteria, sorted by what each actually needs

This is the part worth settling before any code, because it decides what can be finished when.
The chapter lists seven acceptance criteria and they do not all cost the same.

| | Criterion | Needs |
|---|---|---|
| 1 | the instrument is cheaper than everything it measures, by at least an order of magnitude | the board |
| 2 | the minimum agrees with the upstream suite within a stated factor | the board |
| 3 | warm and cold differ, and the chapter says by how much | the board |
| 4 | priority inheritance changes the contended result | the board |
| 5 | two instruments agree on the period, within the acquisition board's resolution | the board **and** the Raspberry Pi with the acquisition board |
| 6 | the disagreement is detectable: with the period deliberately changed, the reduction reports it | **a host only** |
| 7 | no row claims the acquisition board where it cannot see | **a host only** |

**Criteria 6 and 7 need no hardware at all**, and they are not the leftovers. Criterion 6 is the
one that makes criterion 5 worth anything: a reduction that cannot report a disagreement would
pass criterion 5 by being blind, and a check that cannot fail has shown nothing. Criterion 7 is
a rule about the eventual table rather than about any run, so it can be enforced from the day
the table exists, and it is the rule most likely to be broken quietly later by somebody adding a
row in a hurry.

So the order of work is: the reduction and its refutation, then the rule over the table, then
the harness cross built for the target, then the bench. That is not the order the chapter lists
them in, and the chapter is not wrong: the criteria are listed by what they mean and this table
sorts them by what they cost.

## The trap this project is most likely to produce

**The counter wraps.** At this processor's clock it wraps in about fifteen seconds. A case that
brackets anything slower than that produces a plausible small number, which is the worst kind of
wrong: it is in range, it is stable, and nothing about it looks like an error.

Every bracketed region therefore records the elapsed count **and** a guard that the region was
shorter than the wrap, and a run that cannot prove it prints a refusal rather than a figure. The
same discipline as [P03's device readiness guard](../../03-one-sensor/src/main.c): a value that
might not be a measurement is not printed as one.

## What this design does not claim

**No interrupt-to-thread latency distribution.** The sibling firmware volume builds that over a
million events, and repeating it would be duplication rather than evidence. This chapter prices
primitives against one another, which is a different question.

**No energy.** The power meter answers that and belongs to P10, with a different instrument and
a different chapter.

**No number at all, yet.** Every row reads `not measured` until a run produces it, and the
chapter already says so. A protocol published as a protocol is honest; a protocol published as a
result is not.

## The parts, which are all on the bench

| Part | Role |
|---|---|
| NUCLEO-H7A3ZI-Q | the subject. Its own cycle counter is the instrument for everything below ten microseconds |
| Raspberry Pi with the MCC 118 | the external witness, for the period row only |
| one general-purpose pin and one lead | what the witness watches |

No sensor, no shield and no breakout. This project measures the kernel, and anything else on
the bus would be something else to explain.
