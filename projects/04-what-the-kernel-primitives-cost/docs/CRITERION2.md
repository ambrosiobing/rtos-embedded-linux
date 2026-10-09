# Criterion 2: the mapping and the factor, written before the comparison

**Criterion 2 failed.** The verdict is at the foot of this page, with the numbers. Everything above
it was committed at 08:26 on Friday 9 October 2026 in `75adb59`, before the upstream suite had been
built, and it is left exactly as it was written.

Everything down to the end of the configuration section therefore says "the suite has not been
run", in the present tense, and that tense is the point rather than an oversight. **A factor chosen
after seeing both numbers is not a criterion**, and criterion 2 asks that our minimum agree with
the upstream suite within a *stated* factor. Stated when, is the whole question, and the commit
history is the only thing that can answer it.

What was written first is: which upstream measurement each of our rows may be compared with, the
span each one brackets read off the upstream source, the factor to be judged against, and two
sharper predictions. **Both predictions were wrong, and one was wrong in its direction**, which is
worth more than either would have been right.

## What the upstream suite is

Zephyr's own `tests/benchmarks/latency_measure`, in the tree this project already builds against,
version v4.5.0-rc1 as the boot banner reports it. It measures kernel operation latencies and
reports **averages**, in cycles and in nanoseconds.

This is a better comparison than figures from the open literature, and for one reason: **its
source is readable, so the span each figure brackets can be established rather than inferred from
a name.** A published "context switch: 180 cycles" cannot be reconciled with anything, because
the only thing stated is the name, and the name is the part that does not travel.

## Only two of our seven rows have an upstream counterpart at all

This is the first result of criterion 2 and it arrived before any measurement:

| Our row | Upstream counterpart | Comparable |
|---|---|---|
| block on one object, be signalled, return | `semaphore.give.wake+ctx` | **yes, span for span** |
| yield to an equal-priority ready thread | `thread.yield.preemptive.ctx` | **yes, after a factor of two** |
| block on several objects, be signalled, return | none. The suite has no `k_poll` benchmark | no |
| hand work to a queue rather than do it in place | none. The suite has no work queue benchmark | no |
| contended mutex, priority inheritance on | none. Only `mutex.lock.immediate.recursive`, uncontended | no |
| contended mutex, priority inheritance off | none, same reason | no |
| periodic thread period | none. Not a latency measurement | no |

*Table. Our seven rows against the upstream suite, Friday 9 October 2026.*

**Five rows of seven measure something the upstream suite does not measure**, and that is worth
saying plainly rather than treating as a gap. It answers the question left open in
[BENCH.md](BENCH.md) on Thursday 8 October 2026, which was whether these figures are the same
quantity anybody else quotes. For five of them: **nobody is quoting it.** The table is not
redundant with upstream, and the contended mutex row, the one this project is most pleased with,
has no upstream figure to agree or disagree with.

The nearest misses are worth naming so that nobody later mistakes one for a counterpart.
`events.wait.blocking` and `events.set.wake+ctx` are the same *shape* as our several-objects row
but they are `k_event`, a different object from `k_poll`. `fifo.put.wake+ctx` is the same shape as
our queue hand-off but a FIFO is not a work queue. **A comparison against a different object
would produce a number and the number would be about the difference between the objects.**

## The two spans, read off the upstream source rather than off its names

### `semaphore.give.wake+ctx` is our bracket, endpoint for endpoint

Upstream's `alt_thread` stamps `mid` and then calls `k_sem_give`. Upstream's `start_thread` is
blocked in `k_sem_take`, wakes, and stamps `finish`. The reported figure accumulates
`timing_cycles_get(&mid, &finish)`.

Ours stamps `t_handoff` in the giver thread as its last statement before `k_sem_give`, and the
measurer stamps after `k_sem_take` returns.

**The two spans have the same two endpoints: the instant before the give, and the instant after
the woken thread resumes.** Both therefore contain the give executing, the scheduler choosing,
the switch, and the return from the take. This is the one row in the project where criterion 2 is
a strict test rather than a sanity check.

Two differences remain and neither moves an endpoint. Upstream's giver is released by the
semaphore count; ours is released by a separate `go` semaphore given before the bracket opens.
Upstream's threads sit at its own priorities; ours are measurer 5 and giver 7, the giver lower so
that releasing it does not run it.

### `thread.yield.preemptive.ctx` is one switch and ours is two

Upstream's `start_thread` stamps `start`, calls `k_yield`, and then reads the sample that
`alt_thread` stamped **as its first statement after being switched in**. The yield back to
`start_thread` happens after that stamp and falls outside the span.

**So the upstream figure is one context switch.** Ours brackets `k_yield` and closes after control
has returned, which is two.

This had to be read rather than assumed. A summary of that file, consulted first, asserted a round
trip of two switches in its headline while describing a one-switch span in its body, and the
factor the whole comparison turns on would have come out inverted. The source settles it and the
source is four lines long.

## The factor, and the predictions, both stated now

The criterion and the prediction are deliberately two different things. **The criterion is
generous, because it is asking whether two measurements are about the same quantity. The
prediction is sharp, because it is asking whether this project understands its own brackets.** A
prediction can fail without the criterion failing, and that is a useful outcome rather than a
contradiction.

**Criterion 2 is met if, for both comparable rows, our figure and the upstream figure agree within
a factor of 1.5 after the stated span correction.** For `block on one object` the correction is
none. For `yield` the correction is to halve ours.

The predictions, sharper:

| Row | Prediction |
|---|---|
| block on one object against `semaphore.give.wake+ctx` | ours lands between 0.7 and 1.0 times the upstream average |
| yield, halved, against `thread.yield.preemptive.ctx` | ours halved lands within 10 per cent of the upstream average, and below it |

**Both predictions say "below", and that direction is not a hedge: it is forced by the
statistics.** Upstream reports an average over its iterations and we report a minimum over
sixty-four. A minimum cannot exceed an average of the same quantity. So **our figure coming out
*above* upstream's would mean the spans are not the same quantity after all**, and that is the
specific way this criterion can fail. A figure below tells us much less, which is why the bounds
above are two-sided.

## One instrument check this comparison gets for free

The upstream suite reports both cycles and nanoseconds, so it states its own cycles-per-second
implicitly. Ours is gated against 280 MHz on every run and agrees to four parts in ten thousand.

**If the suite's own ratio also implies 280 MHz, the two are counting the same counter** and the
comparison is valid in cycles as well as in nanoseconds. If it does not, the comparison must be
made in nanoseconds only, because then the word "cycle" means two different things on the two
sides and comparing the counts would be the same error criterion 7 exists to prevent.

The comparison will therefore be reported **in nanoseconds**, with the ratio check stated beside
it. That costs nothing and removes a way to be quietly wrong.

## A configuration difference found while reading upstream's `prj.conf`, and closed

**Upstream sets `CONFIG_TIMESLICING=n`. Ours did not set it at all, and it defaults to `y` with
`CONFIG_TIMESLICE_SIZE` defaulting to 20 ms.** So every figure this project published before
Friday 9 October 2026 was taken with a 20 ms timeslice compiled in and enabled.

That was never a decision. It was a default nobody read, in a project whose entire subject is what
kernel primitives cost.

It matters in two separate ways and only one of them changes a number.

**For criterion 2 it is disqualifying while it stands.** The scheduler carries slice bookkeeping
in the path every bracket here contains. Comparing our figures against a suite that compiles that
code out would be comparing two kernels configured differently, which is precisely the error the
design warns about in its opening section: a table reading "Zephyr 180, FreeRTOS 210" comparing
two sets of defaults and saying almost nothing. So `CONFIG_TIMESLICING=n` is now set in our
`prj.conf`, matching upstream, and the four rows already in [RESULTS.md](RESULTS.md) are
re-measured on the matched configuration.

**For our own published minima it changes nothing, and the arithmetic says why rather than the
hope.** The yield case runs its partner at the measurer's own priority, which is exactly the
arrangement timeslicing acts on. But sixty-four brackets of about 9 microseconds is under 600
microseconds in total, against a 20 millisecond slice, so no slice boundary could fall inside any
run. The distributions bear that out: the yield row reads 2615 in sixty of sixty-four samples with
no long outlier anywhere. **The code was in the path and the behaviour never fired.**

The figures will still move, because the build changed, and they should move by less than the two
per cent bound rather than by nothing.

### The settings are now printed by the run

`report_config()` prints `CONFIG_TIMESLICING`, its slice size, `CONFIG_ASSERT` and `CONFIG_POLL`
beside the cache report. **A Kconfig default read out of the upstream tree is a claim about a
version; a line printed by the binary is a statement about what is running.** The same reasoning
that put `report_caches()` in, and the same reasoning that found this: ask the build rather than
reason about it.

`CONFIG_ASSERT` was already `n` on both sides, which is the one setting that had been thought
about.

## The risk that would stop this, named before the attempt

**The suite needs Zephyr's timing API, and whether it is supported on `nucleo_h7a3zi_q` is not
established here.** If it is not, the build fails or the figures are refused, and criterion 2
cannot be run as designed.

If that happens it is recorded as not run, with the reason, rather than worked around by
substituting a different upstream suite or by hand-porting the benchmark. **A hand-ported
benchmark is our code again, and comparing our code with our code is not what criterion 2 asks
for.** The fallback in that case is to state criterion 2 as blocked on the timing API and leave it
blocked, which is a smaller loss than it sounds given that five of the seven rows have no upstream
counterpart in any case.

## The upstream run, Friday 9 October 2026 at 08:46: criterion 2 fails

The suite built and ran. **The named risk did not materialise**: `CONFIG_TIMING_FUNCTIONS=y` is
supported on `nucleo_h7a3zi_q`, the build completed at 40352 bytes of flash, and the run ended
`PROJECT EXECUTION SUCCESSFUL` with forty-eight rows.

| Row | Upstream | Ours | Ratio |
|---|---|---|---|
| `thread.yield.preemptive.ctx.k_to_k` | 188 cycles, **671 ns** | 1295 counts, **4625 ns** | **6.9** |
| `semaphore.give.wake+ctx.k_to_k` | 290 cycles, **1037 ns** | 1461 counts, **5218 ns** | **5.0** |

*Table. The two comparable rows. Ours are from the 08:03 run, with the yield halved as
[CRITERION2.md](CRITERION2.md) specified before the comparison.*

**Criterion 2 asked for agreement within a factor of 1.5. The factors are 6.9 and 5.0, so
criterion 2 fails**, and it fails by enough that no reading of the numbers rescues it.

### Both predictions were wrong, and the direction is the more informative error

The predictions said upstream should land **at or above** ours, because upstream reports an
average and we report a minimum of sixty-four, and a minimum cannot exceed an average of the same
quantity. **Upstream came in five to seven times below.**

That argument was sound and its conclusion is refuted, which leaves exactly one reading: **the two
spans are not the same quantity**, whatever their endpoints say. The endpoints were established
from the upstream source, line by line, and they do match. So the difference is not in where the
brackets open and close. It is in what is inside them, or in what measures them.

### It is not a unit error, and the check that establishes that was built in beforehand

[CRITERION2.md](CRITERION2.md) said the comparison would be reported in nanoseconds with the
suite's own cycles-to-nanoseconds ratio checked against 280 MHz beside it, so that comparing
counts would only be done if both sides count the same counter.

    188 cycles / 671 ns   = 280.2 MHz
    290 cycles / 1037 ns  = 279.7 MHz

**Upstream's counter ticks at 280 MHz and so does ours**, gated on every run to four parts in ten
thousand. Both sides count the same rate, the comparison is valid in cycles as well as in
nanoseconds, and the factor of five is real rather than a mislabelled unit.

### The residuals agree with each other, which is the one real clue

Our figures do not look like upstream's times a factor. **They look like upstream's plus a
constant.**

| Row | Ours | Upstream | Ours less upstream |
|---|---|---|---|
| one context switch | 1295 | 188 | **1107** |
| semaphore wake and return | 1461 | 290 | **1171** |

**Those two residuals agree to 64 counts, which is 5.6 per cent of their mean.** The two
operations have different paths inside them and the offset does not change, which is what an
additive cost outside the operation looks like rather than a path that is genuinely slower.

It also explains a shape that had gone unremarked. Upstream says a bare switch at 188 is much
cheaper than a semaphore wake at 290, a ratio of 0.65. Ours says 1295 against 1461, a ratio of
0.89, nearly equal. **A large constant added to both would flatten exactly that way**, and the
flattening is visible in our own table without any reference to upstream.

**The additive model does not fit the third row, and that is recorded rather than set aside.**
`hand work to a queue` reads 1196, and subtracting 1140 leaves 56 counts for a work submit plus a
dispatch, against upstream's 339 for the analogous `fifo.put.wake+ctx`. 56 counts is implausibly
small. So the offset is not simply present in every bracket, and **no mechanism is claimed here.**
Two residuals agreeing is a clue and three operations are not enough to fit a model to.

### Criterion 1's verdict is now in doubt, and that follows directly

Criterion 1 asks that the instrument be cheaper than everything it measures by at least an order
of magnitude. Every run has reported it met: 81 counts against a smallest measured figure of 1196,
a ratio of 14.

**That division used our own figures on both sides.** Upstream's comparable span is 290 cycles, so
our 81-count instrument against the operation as upstream prices it is a ratio of **3.6**, which is
not an order of magnitude. And upstream's `semaphore.give.immediate` is 47 cycles, so **our empty
bracket costs more than a whole semaphore give.**

Criterion 1 is not withdrawn, because its arithmetic over the figures this project actually took is
correct. **What is withdrawn is the comfort.** Its verdict depends on the figures being
measurements of the operations rather than of the operations plus an offset, and criterion 2 has
just put that in question. If the offset is real, criterion 1 fails.

This is the same error pattern the log already records twice: a check whose two sides come from
the same suspect source cannot find a fault in that source.

### What this does and does not touch elsewhere

**Criterion 3 is unaffected.** It compares warm against cold within one build using one
instrument, so a constant common to both arms cancels in the difference. The penalties of 725,
843, 944 and 1004 counts are differences and survive. Their **percentages do not**, because those
divide by a baseline that may carry the offset.

**Criterion 4's ratio survives and one of its explanations does not.** The 101 microsecond
difference is a difference and stands. But its arithmetic read 4010 counts of residual as "about
three context switches at the 1300 counts already measured for one". At upstream's 188 cycles per
switch, 4010 counts is twenty-one switches, not three. **That reading was self-consistent and
anchored to a figure now in question**, so it is withdrawn as an explanation while the number
stays.

**The configuration was still not matched for this comparison.** The 08:03 figures were taken with
`CONFIG_TIMESLICING=y` and a 20 ms slice, which `5bd71db` set to `n` to match upstream, and that
rebuilt image has not yet been run. It cannot account for a factor of five, so the failure above
stands, but the matched run is owed and is not a substitute for the test below.

### The decisive next test, one variable

**Replace `measure_now()` with the timing API that upstream uses, change nothing else, and run the
same seven operations.** Ours calls `k_cycle_get_32()`; upstream calls `timing_timestamp_get()`,
which on this part reads the cycle counter directly, and the suite additionally measures and
subtracts its own timestamp overhead in `timing_sc.c`.

If our figures fall toward upstream's, the difference was the instrument and this project has been
pricing its own measuring apparatus. If they do not move, the difference is inside the brackets and
the next suspect is the configuration, starting with `CONFIG_POLL=y`, which we set and upstream
does not, and which puts poll notification into every semaphore give.

**One variable, and it can fail either way**, which is the only reason it is worth a build.

## The second comparison, Friday 9 October 2026 at 09:40: still failing, from 6.9 to 1.84

The instrument swapped for the one upstream uses, nothing else about the measurement changed.

| Row | Upstream | Ours, old instrument | Ours, timing API | Factor now |
|---|---|---|---|---|
| `thread.yield.preemptive.ctx` | 188 cycles | 1295 (6.9 times) | **345.5** | **1.84** |
| `semaphore.give.wake+ctx` | 290 cycles | 1461 (5.0 times) | **500** | **1.72** |

*Table. Ours halved for the yield, as specified before any comparison. Figures are warm minima of
sixty-four against upstream's averages.*

**Criterion 2 asked for a factor of 1.5 and the factors are 1.84 and 1.72, so it still fails.**
That is said first and plainly, because 1.84 is close enough to 1.5 to invite rounding and the
criterion was stated in `75adb59` before any number existed, which is the only thing that makes it
worth anything.

**The instrument was most of the gap.** From 6.9 and 5.0 to 1.84 and 1.72, so roughly four fifths
of the discrepancy was the stopwatch rather than the kernel. Subtracting our instrument's remaining
23 counts, which upstream subtracts and we do not, gives 1.78 and 1.64. Still outside.

### The residual changed shape, and the direction of that change is the finding

Before, the **differences** agreed: 1107 and 1171 counts, within 5.6 per cent, which suggested a
cost sitting outside the operation. Now the **ratios** agree better than the differences do:

| | Difference from upstream | Ratio to upstream |
|---|---|---|
| one context switch | 157.5 | 1.84 |
| semaphore wake and return | 210 | 1.72 |
| agreement between the two | 33 per cent apart | 7 per cent apart |

**A proportional residual on two paths whose only common content is a context switch points at the
switch.** Neither model fits well enough to be called one, and with two comparable rows it cannot
be settled by arithmetic. It is named here as the shape of the thing rather than as its cause.

### `CONFIG_POLL` is eliminated, from data already in hand

The previous page named `CONFIG_POLL=y` as the next suspect: we set it for the several-objects row
and upstream does not, and it puts poll notification into every `k_sem_give`.

**The yield row rules it out as the common cause, and no new run was needed to see it.** The yield
bracket contains `k_yield` and no semaphore give at all, so poll notification cannot be inside it,
yet the yield gap of 1.84 is **larger** than the semaphore gap of 1.72. A cost present only in the
semaphore path cannot explain a larger discrepancy in a path that does not contain it.

It may still contribute to the semaphore row. It is no longer the leading candidate.

### What upstream turns off that we never matched

Reading its `prj.conf` again, with the question "what does it disable that costs something on every
context switch", gives a better candidate than speculation did:

    CONFIG_TEST_HW_STACK_PROTECTION=n
    # Disable HW Stack Protection (see #28664)
    CONFIG_HW_STACK_PROTECTION=n

**Upstream disables hardware stack protection explicitly, with a comment, and we never set it at
all.** On this part that is an MPU region reprogrammed on every thread switch. It is a per-switch
cost, it is in the path of every row in our table, and it is present in our build only because a
default went unread, which is the second time today the same thing has happened.

`CONFIG_PM=n` is the other setting upstream states and we do not.

**This is a better class of candidate than the ones before it, and for a reason worth naming: it
comes from reading what the other side configured rather than from reasoning about what might
differ.** Upstream wrote down what it turned off. Matching that list is cheaper and more likely to
be right than any model of the residual.

### What the next run has to show, stated before it runs

- `CONFIG_HW_STACK_PROTECTION` and `CONFIG_PM` printed by the run, so the configuration is on the
  record rather than inferred from a default
- with both matched to upstream, **the factors fall below 1.5 and criterion 2 is met**, if an MPU
  reprogramming per switch is what the remaining proportional residual is
- and if they do not fall, the candidate is wrong and the residual is somewhere that reading
  upstream's configuration cannot find, at which point the honest move is to stop and record
  criterion 2 as failing with the gap characterised rather than keep hunting

**That last clause is the one that matters.** Three rounds of this have each closed most of a gap,
and a project can spend an arbitrary number of evenings chasing a factor of 1.7. The criterion was
written to be answerable, and "fails by 1.7, with the instrument and two configuration settings
accounted for" is a publishable answer.
