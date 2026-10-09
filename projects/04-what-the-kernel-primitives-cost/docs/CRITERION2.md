# Criterion 2: the mapping and the factor, written before the comparison

**No upstream number appears on this page.** The suite has not been built or run. What is written
here is which upstream measurement each of our rows may be compared with, the span each one
brackets read off the upstream source, and the factor the criterion will be judged against.

That order is the point. **A factor chosen after seeing both numbers is not a criterion**, and
criterion 2 asks that our minimum agree with the upstream suite within a *stated* factor. Stated
when, is the whole question.

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
