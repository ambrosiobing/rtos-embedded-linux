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

## Criterion 4, settled Thursday 8 October 2026: what priority inversion costs

The contended case ran for the first time. Three threads: the measurer at priority 5 wanting the
lock, a medium thread at 6 that is runnable and holds nothing, and a holder at 7 inside a
bounded amount of work with the lock.

| Arm | Steady, counts | Steady | Spread across 64 samples |
|---|---|---|---|
| priority inheritance on, a `k_mutex` | 11100 | **39.6 us** | 11061 to 11192 |
| priority inheritance off, a binary semaphore | 39450 | **140.9 us** | 39409 to 39474 |

**The difference is 28350 counts, or 101 microseconds.** The wait is 3.55 times longer without
inheritance, and the criterion asked only that the two differ, so **criterion 4 is met**.

That is the textbook result with a number attached. Without the protocol the high-priority
thread waits for essentially the whole of an unrelated thread's work, and here that is 101 us
added to a 39.6 us wait.

### The arithmetic closes on itself, which is why the figure is believable

Medium is given 8000 spin iterations and the holder 2000. If the only difference between the
arms is medium's work, then:

    28350 counts of difference / 8000 iterations  =  3.54 counts per iteration

Apply that same cost to the holder's 2000 iterations and its work should be about 7090 counts.
Subtract it from the inheriting arm:

    11100 - 7090  =  4010 counts of something that is not spinning

**4010 counts is about three context switches** at the 1300 counts already measured for one
earlier this evening, and three is what the sequence contains: the measurer blocking, the
boosted holder resuming, and the measurer being woken on the release. Two independent
measurements taken for different purposes agree, which is a check rather than a coincidence.

### The prediction was close and slightly high, and the reason is identifiable

The prediction written before the run was a factor of **four or five**, from medium being given
four times the holder's work. The measurement says **3.55**.

The gap is the fixed cost. Both arms carry the same 4010 counts of switching and locking, which
does not scale with the work, so it dilutes the ratio: the work alone is in the ratio 5 to 1 and
the measured totals are in the ratio 3.55 to 1. **A prediction about the work was applied to a
total that also contains overhead**, which is a small error of the kind worth recording, because
the same mistake in a chapter would read as the inheritance protocol being less effective than
it is.

**The right statement for the chapter is the difference, not the ratio.** 101 microseconds is
medium's work, whatever the holder was doing, and it transfers to any other case where the
question is what an unrelated thread costs you.

### The other four rows reproduced

| Operation | Second run | Third run | Change |
|---|---|---|---|
| hand work to a queue | 1200 | 1201 | 0.1 per cent |
| block on one object | 1460 | 1476 | 1.1 per cent |
| block on several objects | 1803 | 1806 | 0.2 per cent |
| yield round trip | 2600 | 2598 | 0.1 per cent |

All inside the **two per cent** bound this project set for itself after the previous pair of
runs, which is the first time that bound has been used rather than merely stated.

## The cache explanation is withdrawn, and the hardware was asked rather than assumed

Three times in this log the slow context switch was attributed to the caches and the flash wait
states on this part. The run of Thursday 8 October 2026 at 22:48 asked the processor instead of
reasoning about it:

    # caches, as the build asked and as the hardware reports
    #   the build asked for CONFIG_ICACHE y and CONFIG_DCACHE y
    #   the control register says instruction cache ON, data cache ON

**Both caches are on, and the build and the hardware agree.** The explanation is refuted and is
withdrawn here rather than quietly dropped.

Two things follow, and the second is more useful than the first.

**Criterion 3 is not retired, it is worth building.** With both caches on there really are two
states to compare, so warm against cold is a measurement rather than a formality, and the
warm-up visible in every operation's first samples has something behind it.

**The figure is probably not what I have been calling it.** About 1300 counts has been described
in this log as "a single context switch", and that is an inference from the yield case being
roughly twice the block-on-one case rather than a statement about what the bracket contains.
`block on one object` runs from the giver's stamp to the measurer's resumption. That is a
**complete wake-up path**: the give executing, the scheduler choosing, the switch itself, and
the return from the take. A published context-switch figure usually measures something a good
deal tighter.

So the open question has moved. It is no longer "why is the switch slow" but **"is this the same
quantity anybody else is quoting"**, and that is exactly what criterion 2 is for. The comparison
has to be like for like or the disagreement it finds will be about definitions rather than about
this board.

The lesson for the volume is the one already in its own rules and broken here anyway: **a
plausible cause that nothing has tested is a guess**, and this one survived three separate
write-ups because it sounded right. The processor could have been asked at any point, and
answering took eight lines.

## What this run does not settle, and why no row is filled in

**Criterion 1, the instrument being cheaper than everything it measures.** Two reads of the
counter sit inside every bracket and their cost has never been measured. At 1217 cycles for the
cheapest operation there is plenty of room, but "plenty of room" is not a number and the
criterion asks for an order of magnitude.

**Criterion 2, agreement with published figures within a stated factor.** About 1300 cycles for
a single context switch is 4.6 us at 280 MHz, which is **slow for a Cortex-M7** and is the one
figure here I would not publish without a comparison. **The explanation offered here was the
caches, and it is wrong: see the withdrawal below, where the control register was asked and
said both are on.**

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

## Criterion 3, Friday 9 October 2026 at 07:37: measured, and one row of it refuted its own case

The instruction cache invalidated before every bracket, the invalidation outside it, minimum of
sixty-four in both arms. The board had been powered off and was switched on about a minute
before the console opened.

| Operation | Warm, counts | Cold, counts | Penalty, counts | Penalty | Cold is higher by |
|---|---|---|---|---|---|
| hand work to a queue rather than do it in place | 1201 | 2113 | 912 | **3.26 us** | 75 per cent |
| yield to an equal-priority ready thread | 2584 | 3318 | 734 | **2.62 us** | 28 per cent |
| block on several objects, be signalled, return | 1797 | 1969 | 172 | 0.61 us | 9 per cent |
| block on one object, be signalled, return | 1472 | 1468 | none | none | **COLD IS LOWER** |

*Table. The four operations warm and cold, Friday 9 October 2026. The rows are sorted by
penalty rather than by cost, because the penalty is what the criterion is about.*

### The fourth row is the one worth reading first, and the flag did its job

The run printed `COLD IS LOWER, which should not happen` for `block on one object`, which is the
branch written into the case on Thursday 8 October 2026 for exactly this outcome rather than a
line that hides it. **It is not a glitch and it is not evidence that cold caches are free. It is
the case reporting that the manipulation does not reach that row's bracket.**

Read the four bracket bodies and the split in the table is exact:

| Operation | Where the bracket opens | Untimed code between the invalidation and the opening | Penalty |
|---|---|---|---|
| hand work to a queue | `t0 = measure_now()` in the measurer, the next statement after `chill()` | none | 912 |
| yield to an equal-priority thread | `t0 = measure_now()` in the measurer, the next statement after `chill()` | none | 734 |
| block on several objects | `t_handoff`, stamped in the **giver** thread | a whole `k_sem_give`, a `k_poll` that blocks, and the giver's wake-up | 172 |
| block on one object | `t_handoff`, stamped in the **giver** thread | a whole `k_sem_give`, a `k_sem_take` that blocks, and the giver's wake-up | none |

**The two operations that bracket from their own timestamp, taken immediately after the
invalidation, show 28 and 75 per cent. The two that bracket from a timestamp taken in another
thread, after a give and a block have already run untimed, show 9 per cent and nothing.** That
is not a pattern found by looking at the numbers and then explaining them afterwards. It is what
the code predicts, and the prediction divides the table along the same line the data does.

The mechanism is ordinary once stated. For the two blocking rows the invalidation happens, and
then the measurer executes a complete semaphore give and a complete blocking take, and the giver
runs its own wake-up path, all of it before the bracket opens. **That untimed run-up walks the
same kernel code the bracket is about, so it re-fills the cache the invalidation just emptied.**
By the time `t_handoff` is stamped the path is warm again.

### What is still unexplained, and what would settle it

`block on one object` came out four counts **lower** cold than warm, which is 0.3 per cent and
well inside the two per cent bound this project set for itself. So the first candidate is that it
is not a difference at all. Two further things are known and neither is sufficient:

**The warm figure for that row is its own first sample.** The warm pass reads 1472, 1488, 1487,
1491, then 1493 for the remaining sixty. It is the only one of the four operations whose first
sample is its **lowest**; every other row's first sample is its highest. So `warm_min` for this
row is a single unrepeated value and the steady warm figure is 1493. Against 1493 the cold
minimum of 1468 is lower by 1.7 per cent, which is still inside the bound but no longer
negligible.

**The cold pass prints only its minimum, so there is nothing to inspect.** Whether 1468 is the
floor of a tight distribution or one low sample in a scattered one cannot be told from this
output. That is an instrumentation gap in the criterion-3 code rather than a question about the
kernel, and it is the first thing to repair: **the cold arm should emit its counts exactly as the
warm arm does.** A minimum without its distribution was enough for the instrument's own cost,
where the floor is the whole point, and it is not enough for a comparison.

### The penalty in counts is the figure to quote, not the percentage

912 counts and 734 counts, on the two rows where the manipulation reaches the bracket. Those are
3.26 and 2.62 microseconds, and they are the same order of magnitude, which is what a fixed cost
of re-fetching a code path should look like. **The percentages differ by a factor of nearly three
only because the baselines differ**, the queue hand-off being the cheapest operation in the table.

This is the second time in two days the same lesson has come up. Criterion 4's record above says
the chapter should quote the 101 microsecond difference rather than the 3.55 ratio, for the same
reason: a ratio carries whatever fixed cost sits in both arms, and a difference does not.

### One figure crossed a boundary the table did not anticipate

**The cold yield round trip is 11.85 microseconds, and the witness on this bench resolves about
ten.** Every bracketed row in [RESULTS.md](RESULTS.md) carries `Scale` `below`, a prediction
written when only warm figures were imagined, and
[check_instruments.py](../../../scripts/check_instruments.py) refuses a `below` row whose measured
duration reaches the resolution. So that row cannot be entered as the table now stands, and the
check is right to say so rather than being loosened.

The prediction is falsified for one row of twenty-six, and the falsification is specific: warm
yield at 9.23 microseconds sits eight per cent under the boundary and cold yield at 11.85 sits
nineteen per cent over it. **The `Scale` column predicts per operation, and the quantity it
describes turns out to depend on the cache state as well**, which is a question about the table's
shape. It is left open here rather than answered by relabelling a row.

Two rows are therefore filled on Friday 9 October 2026 and two are not. `hand work to a queue`
has both arms sound and both under the boundary, so warm and cold both go in. `yield to an
equal-priority ready thread` has a sound pair of measurements and nowhere to put the cold one.
The two blocking rows have a warm figure and a cold figure that measures the wrong thing, so
neither goes in.

### Four presses of the black RESET button, and the reproducibility they bought

The black RESET button was pressed three times at about fifteen second intervals, and the console
also delivered the run buffered from the power-on a minute earlier. **Four complete runs, and all
four are identical byte for byte**: 5153 bytes each, every `a_counts` line matching digit for
digit, including the first samples.

That is the control this log has been missing. The record above, written Thursday 8 October 2026,
observed that two builds differing only in code that nothing executes moved every figure by about
one per cent, and called that the signature of instruction placement rather than of the kernel.
**It was an inference from two points and it now has its control: within one build, across four
resets, the counts do not move at all.** The variation between builds is therefore placement and
not run-to-run noise, and the two per cent bound is a bound on comparing builds rather than on
repeating a measurement.

The warm figures also reproduce across the build boundary, which is the third time that bound has
held:

| Operation | Second run | Third run | Fourth run | Widest spread |
|---|---|---|---|---|
| hand work to a queue | 1200 | 1201 | 1201 | 0.1 per cent |
| block on one object | 1460 | 1476 | 1472 | 1.1 per cent |
| block on several objects | 1803 | 1806 | 1797 | 0.5 per cent |
| yield round trip | 2600 | 2598 | 2584 | 0.6 per cent |

A fifth boot banner appeared in the log with one line of output after it before the next banner
arrived, and the byte count identifies it rather than leaving it a mystery: that chunk is exactly
82 bytes longer than the others, and the banner plus `# p04 under zephyr` with their line endings
is exactly 82 bytes. **One press of the button produced two resets inside half a second**, so the
first was cut short before it could print anything further. A contact bounce, visible only
because the output is self-identifying and the byte counts are printed as the capture runs.

### A defect found while reading the code rather than the output

The warm pass accumulates two minima, and the inner loop sits inside the outer one:

    for (size_t k = 0u; k < SAMPLES; k++) {
            if (counts[k] < smallest) { ... }

    for (size_t k = 0u; k < SAMPLES; k++) {
            if (counts[k] < warm_min[op]) { ... }
    }
    }

It runs 4096 iterations where 64 are wanted. **The numbers are unaffected**, because a minimum is
idempotent and the inner loop recomputes the same answer sixty-four times, so nothing above is in
doubt. It is a brace misplaced when `warm_min` was added, it runs outside every bracket, and it is
repaired in the next commit rather than left in place because it is harmless.
