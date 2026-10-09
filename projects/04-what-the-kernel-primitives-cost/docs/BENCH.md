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

> Written Thursday 8 October 2026 and **superseded on Friday 9 October 2026**, when the leads were
> fitted. The sentence is left standing because this section is a dated record of what one run did
> not settle, and rewriting it would make that run look better informed than it was. See
> [the witness is wired](#the-witness-is-wired-friday-9-october-2026) at the foot of this page.

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

## Criterion 3 again, Friday 9 October 2026 at 08:03: settled, with the control holding

Built at 08:02:42 and the banner says so, which is the first run in this project able to prove
it is the code just written. The previous capture at 07:53 was an exact repeat of the one at
07:37 because a rebuild had been skipped, and the application's own name could not see that.

| Operation | Warm, counts | Cold, counts | Penalty, counts | Penalty | Cold is higher by |
|---|---|---|---|---|---|
| block on several objects, be signalled, return | 1800 | 2804 | **1004** | 3.59 us | 55 per cent |
| hand work to a queue rather than do it in place | 1196 | 2140 | **944** | 3.37 us | 78 per cent |
| block on one object, be signalled, return | 1461 | 2304 | **843** | 3.01 us | 57 per cent |
| yield to an equal-priority ready thread | 2590 | 3315 | **725** | 2.59 us | 27 per cent |

*Table. Criterion 3 with the invalidation at each bracket's own opening, Friday 9 October 2026.
Sorted by penalty. Minimum of sixty-four in both arms.*

**Criterion 3 is met.** Warm and cold differ on all four operations and the chapter can say by
how much.

### The predictions written before the run, and how they came out

The record of 07:37 above stated three predictions before this build was pushed. They are
repeated here with the outcome, because a prediction recorded and then not scored is decoration:

| Prediction | Outcome |
|---|---|
| `block on one object` cold lands roughly 2200 to 2400 | **2304.** Inside |
| `block on several objects` cold lands roughly 2500 to 2700 | **2804.** Above, by 104 counts |
| `yield` and `hand work to a queue` do not move at all, from 3318 and 2113 | **3315 and 2140.** Moved by 3 counts and 27 counts |

**The third is the one that mattered and it is the control.** The giver thread takes no part in
the yield case or in the work queue case, so a change in those two would have meant the
giver-side invalidation was reaching somewhere it should not. Three counts is 0.09 per cent and
twenty-seven is 1.3 per cent, both inside this project's two per cent build bound. The change is
confined to the two operations it was meant to reach.

The second prediction was low by four per cent, and the reason is identifiable rather than
mysterious: the range was extrapolated from the two penalties then in hand, 734 and 912, and
`k_poll` turns out to carry more distinct code than either of those paths.

### All four penalties now sit in one band, which is the result behind the result

725, 843, 944, 1004. Mean 879 counts, and the widest is 1.39 times the narrowest. **That is what
a fixed cost of re-fetching a code path should look like**, and it is the first evidence in this
project that the quantity being measured is a property of the memory system rather than of each
individual primitive.

The percentages, by contrast, run from 27 to 78 per cent and carry no such structure, because
each divides by a different baseline. **The chapter should quote about 0.9 thousand counts, or
roughly 3 microseconds, as the cold-start cost of a kernel path on this part**, and give the
percentages only as a second column.

### The penalty ranks by how much distinct code the bracket contains, not by what it costs

This is the part that would be easy to get backwards. **The yield round trip is the most
expensive operation in the table and has the smallest cold penalty.**

That is not a defect, and the reason makes it a check rather than a puzzle. The yield bracket
contains a full round trip: out through an equal-priority partner and back, so **the switch path
is traversed twice inside one bracket**. The first traversal runs cold and warms the path; the
second runs warm. A round trip therefore pays the refill once, not twice.

**It is a falsifiable claim and the number decides it.** If yield paid twice, its penalty would
be about 1500 to 1700. If it pays once, its penalty should be of the same order as a single
wake-up's, which is `block on one object` at 843. It measured 725. The claim survives.

The ordering of the other three follows the same reading. `k_poll` over two objects is more code
than `k_sem_take` over one, and it has the largest penalty at 1004. The work queue hand-off
carries a submit and a dispatch, and sits at 944.

### The choice of statistic turns out not to be load-bearing, and now that can be shown

The cold pass prints its distributions in this build, which the previous one did not, and that
was the open question left at 07:37: whether a cold minimum was a floor or a single low sample.
Both can now be read:

| Operation | Cold minimum | Cold steady | Penalty from minima | Penalty from steady |
|---|---|---|---|---|
| yield round trip | 3315 | 3338 | 725 | 723 |
| block on one object | 2304 | 2307 to 2308 | 843 | about 835 |
| block on several objects | 2804 | 2804, all sixty-four | 1004 | about 997 |
| hand work to a queue | 2140 | 2146 | 944 | 950 |

**The two statistics agree to about one per cent, so the choice between them does not carry this
result.** Saying so is worth more than defending the minimum, because the concern raised at 07:37
was real and this is what retires it.

`block on several objects` reads 2804 in sixty-four samples of sixty-four, which is the most
degenerate distribution this project has produced.

### An observation not explained, recorded as such

**The first cold sample is the lowest in every one of the four operations**, by 23, 3, 0 and 6
counts. That is the mirror image of the warm pass, where the first sample is the highest in every
operation. So in the cold arm the minimum is the first sample, which is exactly the property
criticised in the warm arm at 07:37.

It changes nothing here, because the table above shows the minima and the steady values agreeing
to one per cent either way. **No mechanism is offered.** One observation is not a mechanism, the
effect is between 0 and 0.7 per cent, and a candidate involving the branch predictor or the data
cache carrying over from the warm pass would need a control this run does not have.

### Two of the four cold figures cross the witness resolution, and one of them by a hair

This is now a question the table has to answer rather than a note. The witness on this bench
resolves about ten microseconds, and every bracketed row of [RESULTS.md](RESULTS.md) carries
`Scale` `below`, a prediction written when only warm figures existed:

| Operation | Warm | Cold | Cold against the 10 us boundary |
|---|---|---|---|
| yield round trip | 9.25 us | **11.84 us** | over by 18 per cent |
| block on several objects | 6.43 us | **10.01 us** | over by 0.14 per cent |
| block on one object | 5.22 us | 8.23 us | under |
| hand work to a queue | 4.27 us | 7.64 us | under |

**The `below` prediction holds warm for all four and fails cold for two**, and
[check_instruments.py](../../../scripts/check_instruments.py) refuses both, correctly, under its
rule that a measured duration must agree with its own `Scale`. `block on several objects` cold at
10.014 microseconds lands 0.14 per cent over the line, which is the most awkward place available
and is a useful reminder that a boundary written as a round number is still a boundary.

So two rows go in, `block on one object` and `hand work to a queue`, both arms each, and two
wait on a decision recorded in the next section rather than on a measurement.

### The decision this leaves, stated rather than taken

The `Scale` column predicts per operation. The quantity it describes turns out to depend on the
cache state as well, so on this table a single operation can be `below` warm and `above` cold.
Three resolutions exist and they are not equally good.

**Predict per row rather than per operation.** The table already has one row per cache state, so
`Scale` could simply differ between them. It needs two changes to the check: an `above` row would
have to be allowed to state a cache, and the warm-and-cold pairing rule would have to stop being
keyed on `below`. **Both are loosenings, and loosening a check so that a number fits is the thing
the check exists to prevent.**

**Say what `Scale` is actually for, which is narrower than what it currently claims.** Criterion 7
is that no row may claim an instrument that cannot see what it claims, and the cycle counter can
see everything at this scale. The hazard criterion 7 guards against is one-directional: a row
claiming the *witness* for something too short. A `below` row measuring 10.01 microseconds **with
the cycle counter** has falsified its prediction and has not committed the error criterion 7
exists to catch. On this reading the check should report a falsified prediction distinctly from a
criterion 7 violation, and the table should record the outcome of the prediction rather than be
edited until it agrees.

**Leave both rows unfilled and say why.** Costs nothing, settles nothing, and is where they are
tonight.

The second is the one worth doing, and it is a change to the meaning of a published check rather
than a repair, so it is written here and left for a decision.

### The warm arm reproduced, and one caveat about comparing it across builds

Fourth confirmation of the two per cent bound:

| Operation | 07:37 build | 08:03 build | Change |
|---|---|---|---|
| hand work to a queue | 1201 | 1196 | down 0.4 per cent |
| block on one object | 1472 | 1461 | down 0.7 per cent |
| block on several objects | 1797 | 1800 | up 0.2 per cent |
| yield round trip | 2584 | 2590 | up 0.2 per cent |

**The caveat belongs with the figures rather than after them.** The warm arms of these two builds
do not run identical code. `chill()` now sits in the giver thread, and in the warm pass it still
tests a volatile flag on every iteration before returning. The warm-against-cold comparison
within this build is sound, because both arms run the same code and differ only in the flag. The
warm-against-warm comparison across the two builds does not have that property, and the 0.7 per
cent is therefore a bound on the build rather than a reproduction in the strict sense.

The effect is visible in the output. `block on one object` read a dead-flat 1493 for the last
fifty samples in the old build and now oscillates through 1473, 1468, 1508, 1478, 1474 in a
repeating pattern. One extra test per iteration, in a bracket of about 1470 counts.

Criterion 1 holds: the instrument costs 81 counts at best, the smallest thing measured is 1196,
a ratio of 14, and the criterion asks for ten.

### The button bounce, a second time in one morning

One press was asked for and two resets arrived, three seconds apart, the first truncated partway
through the fourth cold count dump. The same signature as the 82 byte excess identified at 07:37.
**Twice in one morning is a property of the button rather than an accident**, so a single press on
this board should be expected to produce two runs, and the second complete one is the one to read.
It cost nothing here because the whole capture is 12915 bytes and both runs fitted.

## Friday 9 October 2026 at 09:40: the instrument was most of it, and every figure above is superseded

The timing API in place of `k_cycle_get_32()`, nothing else about the measurement changed. Built
at 09:39:47 and the banner says so.

**The gate confirmed its own diagnosis.** The busy window read 55790914 against a predicted
56000000, 99 per cent, where the sleeping window had read 4260. So the counter is a core cycle
counter that stops when the core stops, exactly as the refusal implied, and over a window where
the core keeps running it ticks at 280 MHz.

| Operation | With `k_cycle_get_32` | With the timing API | Fell by |
|---|---|---|---|
| yield to an equal-priority ready thread | 2590 | **691** | 3.75 times |
| block on one object, be signalled, return | 1461 | **500** | 2.92 times |
| block on several objects, be signalled, return | 1800 | **850** | 2.12 times |
| hand work to a queue rather than do it in place | 1196 | **677** | 1.77 times |
| contended mutex, priority inheritance on | 11069 | **8381** | 1.32 times |
| contended mutex, priority inheritance off | 39403 | **36610** | 1.08 times |
| the instrument itself, an empty bracket | 81 | **23** | 3.52 times |

*Table. Warm minima of sixty-four, both instruments, Friday 9 October 2026. The left column is
the 08:03 run and the right the 09:40 run.*

**Every figure this project produced before 09:40 on Friday 9 October 2026 was substantially a
measurement of its own stopwatch.** The cheapest row was 59 per cent instrument; the yield row was
73 per cent.

### The additive model is refuted, and it was refuted by criterion 3 rather than by argument

The criterion 2 page proposed, four hours earlier, that our figures looked like upstream's plus a
constant of about 1140 counts, on the strength of two residuals agreeing to 5.6 per cent. It was
labelled a clue and not a mechanism. **It is now refuted, and the refutation was already available
in data this project held.**

If the old instrument added a constant to every reading, then warm and cold both carry it and
**criterion 3's penalties, being differences, would not have changed at all.** They changed by a
third to a half:

| Operation | Penalty, old instrument | Penalty, timing API | Changed by |
|---|---|---|---|
| yield round trip | 725 | 380 | down 48 per cent |
| block on one object | 843 | 525 | down 38 per cent |
| block on several objects | 1004 | 729 | down 27 per cent |
| hand work to a queue | 944 | 703 | down 26 per cent |

So the old instrument's contribution was not a constant. It was not a clean multiple either: the
warm readings fell by factors of 3.75 to 1.08 and the penalties by 1.9 to 1.34, with no single
factor fitting both. **What `k_cycle_get_32()` was adding is not characterised here and is not
pursued**, because the question this project asks is what kernel primitives cost, and the answer
is to stop measuring with it.

The lesson is narrower and more useful than a model would have been: **two numbers agreeing to 5.6
per cent were enough to suggest a mechanism and nowhere near enough to support one.** Three points
would have refused it immediately, and the third point was sitting in the criterion 3 table.

### Criterion 3 re-measured, and its conclusion survives in a weakened form

| Operation | Warm | Cold | Penalty | |
|---|---|---|---|---|
| block on several objects, be signalled, return | 850 | 1579 | **729 counts, 2.60 us** | 85 per cent |
| hand work to a queue rather than do it in place | 677 | 1380 | **703 counts, 2.51 us** | 103 per cent |
| block on one object, be signalled, return | 500 | 1025 | **525 counts, 1.88 us** | 105 per cent |
| yield to an equal-priority ready thread | 691 | 1071 | **380 counts, 1.36 us** | 54 per cent |

**Criterion 3 is still met**: warm and cold differ on all four and the chapter can say by how much.
Two of the earlier readings do not survive intact.

**The band is wider than it looked.** The four penalties now run 380 to 729, a widest-to-narrowest
ratio of 1.92, where the old instrument gave 725 to 1004 and a ratio of 1.39. The claim that they
sat in one tight band, offered as evidence that the cold cost is a property of the memory system
rather than of each primitive, is **weaker than written and is amended here rather than left.** A
factor of two across four operations is consistent with a per-path cost that happens to be similar
in magnitude, which is a smaller claim.

**The yield row's reading survives and is now sharper.** It remains the most expensive operation
with the smallest penalty, 380 counts against 525 for a single wake-up. The explanation was that a
round trip traverses the switch path twice and pays the refill once, so its penalty should resemble
a single switch's rather than double it. At 380 against 525 that still holds, and the old figures
said 725 against 843, a much narrower gap. **The better instrument strengthened the argument it was
not chosen to test.**

### Criterion 1's doubt is retired, by the only arithmetic that could retire it

An hour earlier criterion 1's verdict was put in doubt: its ratio of 14 had divided our figures by
our figures, and against upstream's pricing of the same span our 81-count instrument was a ratio of
3.6, below the order of magnitude the criterion asks for.

    # instrument: an empty bracket costs 23 counts at best, 38 at worst
    # criterion 1: the instrument costs 23 counts and the smallest
    #   thing it measured was 500, a ratio of 21

**23 counts against upstream's 290-cycle pricing of the semaphore wake is a ratio of 12.6**, which
clears ten without reference to our own figures at all. The doubt is retired and criterion 1 is met
on both arithmetics. The old instrument's 81 counts against the same 290 was 3.6, so **the criterion
was genuinely failing and reporting itself met**, which is what a self-referential check does.

### Criterion 4 is vindicated, including the prediction that looked slightly wrong

This is the result of the morning that changes least and says most.

| | Old instrument | Timing API | Changed by |
|---|---|---|---|
| inheritance on | 11069 | 8381 | 1.32 times |
| inheritance off | 39403 | 36610 | 1.08 times |
| **the difference** | **28334 counts, 101.19 us** | **28229 counts, 100.82 us** | **0.4 per cent** |
| the ratio | 3.55 | **4.37** | up 23 per cent |

**The 101 microseconds survived an instrument change that moved everything else by factors of 1.1
to 3.8.** It is dominated by the medium thread's actual work rather than by kernel overhead, which
is precisely why the record already said the chapter should quote the difference and not the ratio.
That advice was given for the right reason and is now demonstrated rather than argued.

**And the prediction written before the first contended run is now met.** It said inheritance off
should cost about four or five times inheritance on, from medium being given four times the
holder's work. The old instrument said 3.55 and the record called the prediction "close and
slightly high", explaining the gap as fixed overhead diluting the ratio. **The explanation was
right and the correction is 4.37, inside the predicted four to five.** The dilution was the
stopwatch.

The residual arithmetic also improves. 28229 counts over medium's 8000 iterations is 3.53 counts
per iteration, so the holder's 2000 should be about 7059, leaving **1322 counts of the inheriting
arm that is not spinning**. Three switches at this run's 345 counts per switch would be 1036, which
is 78 per cent of it. The old figures gave 4010 counts against 1295 per switch, also about three.
**Both arithmetics land on three and neither closes exactly, so the withdrawal of that explanation
stands as a withdrawal.** What has changed is that it is no longer absurd: at upstream's 188 counts
per switch the old 4010 would have been twenty-one switches.

### Every bracketed figure now sits under the witness resolution, so an open decision closes itself

The question left open at 08:03 was what to do about a `Scale` column that predicts per operation
when the quantity turns out to depend on the cache state too. Two cold figures had crossed the ten
microsecond witness resolution while their rows said `below`.

| Operation | Warm | Cold | Against the 10 us boundary |
|---|---|---|---|
| block on one object | 1.79 us | 3.66 us | both under |
| hand work to a queue | 2.42 us | 4.93 us | both under |
| yield round trip | 2.47 us | 3.83 us | both under, where cold was 11.84 |
| block on several objects | 3.04 us | 5.64 us | both under, where cold was 10.01 |

**All eight figures are under the boundary and the `below` prediction holds for every one.** The
crossing was an artifact of the instrument, so the decision does not have to be taken, and
[RESULTS.md](RESULTS.md) gains all eight rows rather than four.

**The same question returns on a different row, and that is worth recording while it is small.** The
contended mutex figures are 8381 and 36610 counts, which are **29.9 and 130.8 microseconds**, both
far above the witness's ten. Their rows say `below` and that prediction is falsified, and it was
falsified from the first contended run rather than by anything to do with the instrument. Those four
rows stay unmeasured here because the cold pass covers only the first four operations, so nothing
has to be decided today. But the `Scale` column has now mis-predicted two different ways, which
strengthens the case for the resolution recommended in [CRITERION2.md](CRITERION2.md) over the
other two.

## The witness is wired, Friday 9 October 2026

![The NUCLEO-H7A3ZI-Q and the MCC 118 on the Raspberry Pi, two leads between them, recorded
Friday 9 October 2026](figures/z04_as_built.gif)

*Figure. Six seconds across the bench as built, at eight frames a second. The
NUCLEO-H7A3ZI-Q is on the left, the Raspberry Pi carrying the MCC 118 acquisition HAT on the
right, and two leads cross between them.*

**This is the first binary file in the repository and the reason it earned the exception is
narrow: no drawing can show that a wire is actually fitted.** `z04_wiring.svg` in the same folder
says what the connection is meant to be, and it said so before anything was connected. A dated
picture of the bench says what is there. Those are different claims and this project has already
been caught once treating the first as evidence for the second, which is why
[the bench state is not the record](../../03-one-sensor/docs/BENCH.md) exists as a rule.

### What it shows, and what it does not

**It shows**: two boards on the bench together, two leads running from the NUCLEO-H7A3ZI-Q's
headers across to the green screw terminals on the MCC 118, one orange and one dark, and a lit
red indicator on the Pi. That is enough to establish that the criterion 5 wiring is no longer a
plan.

**It does not show which pin.** At 640 pixels across a pan, the header position cannot be counted
and the terminal cannot be read. **So the wiring table for P04 is still owed and this file does
not discharge it.** P03 carries a `docs/WIRING.md` with a row per lead naming both endpoints,
after two separate occasions when naming a destination instead of an endpoint produced an
ambiguity that cost a bench session. P04 has no such page yet, and criterion 5 cannot be reported
without one: a period measured by two instruments is worth nothing if the reader cannot tell which
pin the second instrument was watching.

**A moving picture is weaker evidence than a table and stronger evidence than a drawing**, and it
is worth being exact about where it sits rather than letting it look like documentation.

### What it cost, so that the next one is a decision and not a habit

Before this file the largest thing tracked in the repository was 96 KB, an SVG. This is 3.45 MB,
thirty-six times that, and git keeps a blob after a later commit removes it, so it is permanent in
every clone from here on.

It was 5.47 MB at the first attempt and came down by dropping to eight frames a second and
sixty-four colours, which cost nothing a reader needs: the board's own silkscreen label is still
legible at that size. The source is a 14 MB recording at 1920 by 1080 and thirty frames a second,
and **that file is not in the repository and should not be**: the allowlist refuses `.mp4`, which
was checked rather than assumed.

Two guards changed in the same commit as the file, and the second exists because of the first.
`checks.yml` now admits `gif`, and it now refuses **any** tracked file over four megabytes. A type
admitted without a ceiling is how a documentation repository becomes a download, and a limit
written later is a limit nobody writes.
