# First run on the board, Thursday 8 October 2026

Written the same evening. **Nothing here fills a row of [RESULTS.md](RESULTS.md)**, and the
reasons are at the foot of this page: the cache state of this run is not established, the
instrument's own cost has not been measured, and nothing has been compared with anybody else's
figures. A protocol that produced numbers is still a protocol until those three are done.

## What ran

One image, built for `nucleo_h7a3zi_q`, flashed and reset twice. **The two runs produced
identical output**, which is the first thing worth saying: a measurement that changes between
two presses of the same button is not yet measuring the thing it names.

No wiring at all. One USB cable to the ST-LINK socket, carrying power, the flashing disk and
the console. The five operations are the processor and the kernel talking to themselves.

## The instrument checked itself first, and passed

    # clock check: 200 ms of sleep advanced the counter by 56023503, and 280000000 Hz
    #   predicts 56000000, so the count is 100 per cent of the prediction

This gate was added after the previous run, because `measure_clock_hz()` reports what the kernel
says the hardware rate is while `measure_now()` reads a counter, and **nothing had checked that
those two claims were about the same thing.** A counter advancing at some other rate would have
scaled every figure below by an unknown factor while looking entirely reasonable.

It agrees to four parts in ten thousand. The counts below are times.

## The four bracketed operations

One cycle is 3.5714 ns at this rate. The distributions are close to degenerate, so the steady
value is given rather than a median that would imply more spread than exists.

| Operation | Steady, cycles | Steady | Relative |
|---|---|---|---|
| hand work to a queue rather than do it in place | 1217 | 4.35 us | 1.00 |
| block on one object, be signalled, return | 1478 | 5.28 us | 1.21 |
| block on several objects, be signalled, return | 1817 | 6.49 us | 1.49 |
| yield to an equal-priority ready thread | 2573 | 9.19 us | 2.11 |

**The relative column is the chapter's actual product.** A cost in cycles belongs to this board,
this build and this kernel version; a ratio between two primitives is what a later chapter needs
when it chooses one over the other.

**The four numbers are internally consistent, which is a check and not a coincidence.** The
yield case contains a full round trip, out through an equal-priority partner and back, so it
should be about twice a single wake-up. The block-on-one case contains one wake-up plus the give
that caused it. 2573 against 1478 is a ratio of 1.74, which is the right shape: two switches
against one switch plus a call.

**Waiting on several objects costs 23 per cent more than waiting on one**, which is the first
figure in this project that a later chapter could actually use.

**Handing work to a queue is the cheapest of the four**, which surprised me until the brackets
are read carefully. It is not a round trip: Zephyr's system work queue runs at a cooperative
priority above the measurer, so the submit preempts straight into the handler and the bracket
closes there. It measures one switch, where the yield case measures two.

## The periodic thread is not periodic at one millisecond

    a_counts 308014 307995 307992 307986 ... 308000 308000 308000

308000 cycles is **1.1000 ms**, not the 1 ms the sleep asked for, and it is that value in
sixty-four samples out of sixty-four after the first few.

That is a tick granularity showing through rather than a defect: a sleep is rounded up to the
next tick, and a consistent 1.1 ms says the tick is 100 us and the request always crosses one.
It matters twice over. **Criterion 5 asks that two instruments agree on this period**, and they
will agree on 1.1 ms rather than on 1 ms. And any chapter later describing a 1 kHz release on
this configuration would be describing something that runs at 909 Hz.

## Criterion 3 appeared on its own, in data taken for something else

The first sample of every operation is the largest, and the fourth row shows a warm-up rather
than a single outlier:

| Operation | First | Steady | First is higher by |
|---|---|---|---|
| hand work to a queue | 1382, then 1261, 1229, 1216 | 1217 | 13.6 per cent |
| block on several objects | 1898 | 1817 | 4.5 per cent |
| yield to an equal-priority thread | 2620 | 2573 | 1.8 per cent |
| block on one object | 1485 | 1478 | 0.5 per cent |

**Criterion 3 asks that warm and cold differ and that the chapter say by how much.** This is not
that measurement, because nothing here controlled the cache state deliberately. It is the effect
turning up uninvited in a run that was not looking for it, which is a reason to build the
controlled version rather than a substitute for it.

## Second run, the same evening: criterion 1 settled, and a shift worth noticing

The image gained an instrument-cost measurement and nothing else. It reports an empty bracket,
two reads of the counter with nothing between them, 256 times, and takes the **minimum**: an
empty bracket has a floor and no ceiling, so anything above the floor is interference and the
floor is what the instrument costs.

    # instrument: an empty bracket costs 82 counts at best, 103 at worst
    # criterion 1: the instrument costs 82 counts and the smallest
    #   thing it measured was 1200, a ratio of 14. The criterion asks
    #   for at least ten, so this run meets it

**Criterion 1 is met, and the run evaluated it rather than a person doing the division later.**
A criterion whose verdict is computed by hand is one somebody eventually forgets to compute.

**The margin is thinner than "met" suggests, and that is worth saying plainly.** 82 counts is
**seven per cent** of the cheapest row, and it sits inside every bracket in the table. Here is
what subtracting it does:

| Operation | As measured | Less the instrument | Relative, as measured | Relative, corrected |
|---|---|---|---|---|
| hand work to a queue | 1200 | 1118 | 1.00 | 1.00 |
| block on one object | 1460 | 1378 | 1.22 | 1.23 |
| block on several objects | 1803 | 1721 | 1.50 | 1.54 |
| yield round trip | 2600 | 2518 | 2.17 | 2.25 |

The ratios move by two or three per cent, which is small and is not nothing. The eventual table
will have to say which column it is reporting, and this page records both so that the choice is
visible rather than silent.

### The figures moved between two builds that changed nothing relevant

The only difference between the two runs is the instrument measurement, which executes before
any operation and touches none of them.

| Operation | First run | Second run | Change |
|---|---|---|---|
| hand work to a queue | 1217 | 1200 | down 1.4 per cent |
| block on one object | 1478 | 1460 | down 1.2 per cent |
| block on several objects | 1817 | 1803 | down 0.8 per cent |
| yield round trip | 2573 | 2600 | **up** 1.0 per cent |

**Three went down and one went up, from adding code that none of them executes.** That is the
signature of instruction placement rather than of the kernel: the same operations, laid out
differently in flash, cost slightly differently. It is more evidence for the suspicion already
recorded above, that the caches and the flash wait states dominate these figures, and it is a
direct warning about criterion 2.

It also sets a floor on how precisely any of these numbers can be quoted. **A figure from this
project is good to about two per cent at best**, because recompiling with an unrelated change
moves it by that much, and anything finer would be reporting the build rather than the kernel.

The second run's distributions are also markedly more degenerate than the first: `hand work to a
queue` reads exactly 1200 in sixty-two of sixty-four samples, and `block on one object` exactly
1460 in forty-eight consecutive samples. That is the kind of stability that makes a mean and a
median identical and makes the minimum the only honest statistic for an operation with a floor.

## What this run does not settle, and why no row is filled in

**Criterion 1, the instrument being cheaper than everything it measures.** Two reads of the
counter sit inside every bracket and their cost has never been measured. At 1217 cycles for the
cheapest operation there is plenty of room, but "plenty of room" is not a number and the
criterion asks for an order of magnitude.

**Criterion 2, agreement with published figures within a stated factor.** About 1300 cycles for
a single context switch is 4.6 us at 280 MHz, which is **slow for a Cortex-M7** and is the one
figure here I would not publish without a comparison. The likeliest explanation is the caches
and the flash wait states on this part rather than the kernel, and the honest next step is to
find out rather than to assume: the criterion exists so that a disagreement becomes the finding
instead of an embarrassment.

**Criterion 3, warm against cold.** The effect is visible above and has not been controlled.

**Criterion 4, priority inheritance.** Both mutex rows returned not-supported, by design. They
need three threads sequenced so that the medium-priority one is genuinely runnable while the low
one holds the lock, and a version that did not arrange that would produce two equal numbers and
read as a refutation when it was really a badly built case.

**Criterion 5, the two instruments.** The witness has not been connected. It needs one lead from
one general-purpose pin to the acquisition board on the Raspberry Pi, and it is the only row in
the table that has a second instrument at all.

So [RESULTS.md](RESULTS.md) still reads `not measured` in every row, and that is correct rather
than lazy. **A number obtained is not a number earned.**
