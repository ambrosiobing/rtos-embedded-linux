# P04 results

**All twenty-six rows are measured, as of Saturday 17 October 2026 at 10:09.** Thirteen are
Zephyr's and thirteen are FreeRTOS's. Nothing here reads `not measured`, which it did for nine
days, and the column stays in the page because the refusal it used to carry is the reason the
rest can be read: a blank invites somebody to fill it in and a refusal does not.

The rows do not all come from one run and the caption says which came from where. Six of them
sat in this page's prose for part of a morning while the table refused them, and the four
contended mutex operations were relabelled `above` before they could enter; both stories are
below, because the refusals were right until the rule was read.

## What produced these rows, and the revisions it was at

Neither of the two external trees this project compiles against is in this repository, so a row
here means nothing without them named. **Added on Sunday 18 October 2026**, because until then
the provenance of every board figure on this page lived in a Makefile variable and nowhere a
reader would look.

| What | Pinned at | Checked |
|---|---|---|
| The FreeRTOS kernel | tag `V11.1.0` | the tag is what the clone is fetched at, and `make fetch` prints what the clone reports |
| The board support package, which is the sibling firmware volume | commit `1d6d10f826ee0728ded6533defc468baa2ff0f2a`, of Friday 16 October 2026 | verified by hand on the build machine on Sunday 18 October 2026: `git -C ~/src/firmware-nucleo-h7 rev-parse HEAD` in WSL on the demo laptop returned exactly that commit |

**The board rows therefore match the revision this project states they were taken against**, which
had not been established before and is the kind of claim that is easy to assume. The check was one
command and it agreed.

Two caveats belong with that, since a verified fact invites more confidence than the arrangement
deserves:

- **The pin was abbreviated to seven characters until Sunday 18 October 2026.** An abbreviated
  hash can become ambiguous as the other repository grows, and a checkout on an ambiguous prefix
  does not resolve, so it was widened to all forty. The commit it names did not change.
- **It is now a standing guarantee and was a dated observation for one day.** Until Monday 19
  October 2026 nothing compared either pin on an ordinary build: `make fetch-bsp` reported a
  mismatch and only when it was run, and `make board` did not. Joseph decided one rule should
  apply across the set, so both pins are now checked by `scripts/check_pin.py` as an order-only
  prerequisite of every build here, and a clone off its pin refuses rather than reporting. The
  escape hatch is `ALLOW_ANY_REVISION=1`, which warns and says in the same breath that nothing
  measured may be quoted beside a figure taken at the pin.

That check is tested rather than trusted: `scripts/test_check_pin.py` builds real clones in a
temporary directory and requires a refusal for each of a moved commit, a moved tag, a plain copy
that is not a clone, an absent path and an abbreviated hash. And `scripts/check_vendoring.py`
requires that a Makefile cloning an external source actually calls the gate, because removing the
dependency would otherwise be a one-line change nothing would notice.

**One defect from that change is worth leaving on the record**, because it is the failure this
page exists to guard against. The check was first placed above `all`, and the first target in a
makefile is the default goal, so a bare `make` ran the check, built nothing, and exited zero. All
three projects printed one encouraging line that read exactly like a successful build. The pin
readings from those runs were sound; the builds they appeared to confirm had not happened.
`.DEFAULT_GOAL := all` is now named in each file, since an explicit assignment cannot fail the
way an ordering convention can. The three builds were re-run and completed on Monday 19 October
2026.

## The ninth row, the first of two that two instruments watched

`periodic thread period` under Zephyr went in on **Friday 9 October 2026 at 23:16**, and it is
the row the whole second half of the project existed for. Criterion 5 asks that two instruments
agree on it, and they do:

The processor's own counter: minimum 1099.961, **median 1100.000**, maximum 1100.036
microseconds. The MCC 118 on the Raspberry Pi: minimum 1099.985, **median 1100.504**, maximum
1109.775. That is 64 counts against 63 witness periods, from `p04-burst` and the console log
`p04-20261009-231653.log`, and **the medians differ by 0.504 microseconds against a tolerance of
10**, which is the witness's own resolution.

(Written as prose rather than as a table on purpose. `check_instruments.py` parses the first
pipe table on this page and refuses a header it does not recognise, so a second table here would
either break the check or force it to be loosened. It refused this section once already, which
is the guard doing its job on the person who wrote it.)

**Criterion 5 is met.** With it, all seven are.

**The two cells this row shows are the cycle counter's pair, and the Instrument column says
witness.** That looks like a contradiction and is a limitation of a table with one column per
quantity: a count and a duration have to be one measurement, so the pair shown is the one that
is internally consistent, 308000 counts at the stated 280 MHz. The column names the witness
because criterion 7 is about which instrument may see a row at all, and this is the only row the
witness can see. **The row's actual product is neither number but the 0.504 microseconds between
them**, which is in [BENCH.md](BENCH.md) with the capture it came from.

One more caveat belongs with it. The device's counts are the 64 periods measured during the run;
the witness's edges are a later repeat of the same loop, because the firmware repeats the burst
for as long as it is powered so that no recording has to be landed on a 70 millisecond event.
The comparison is therefore of one periodic process seen twice rather than of the same 64
events, and it is sound only if the process is stationary. That is not assumed: `witness.py`
found seven bursts in this recording and their median periods span 0.69 microseconds.

## The six FreeRTOS rows, and the four the table refuses

The FreeRTOS board image ran for the first time on **Saturday 17 October 2026 at 08:48**, built
08:47:14, from the console log `p04f-20261017-084804.log`. The same campaign as the Zephyr
rows, since both now run [adapter/campaign.c](../adapter/campaign.c): the same two clock gates,
both at 99 per cent, the same instrument cost line at 22 counts, the same cold pass over the
first four operations, and the same warm figure, **the minimum of 64**. The six rows entered are
the yield, the block on one object and the block on several, warm and cold.

**The hand-to-queue pair was measured at 08:48 and refused by the table until the afternoon.**
Its warm figure is 1142 counts, 4.08 microseconds, and its cold figure is **3144
counts, 11.23 microseconds**, on the far side of the witness's ten with the row's `Scale`
reading `below`. The check refused it, which was correct against the rule as written, and the
two ways round it were both the quiet moves the check exists to stop. The rule was then read
rather than worked around: the column predicted which instrument could see the operation, and
it never predicted the cache penalty, so the prediction is now held against the warm figure
only. That refinement is its own commit, before this one, and the pair went in after it.

The contended mutex rows under FreeRTOS are 16060 and 69890 counts, **57.4 and
249.6 microseconds**, inheritance on and off, entered the same afternoon under a `Scale`
of `above`; see the Zephyr pair below for the relabel and why it is loud. The period row went in at **09:27 the same morning**, and it is the second row two instruments
watched. The processor's own counter: minimum 999.868, **median 999.979**, maximum 1000.050
microseconds. The MCC 118 on the Raspberry Pi: minimum 999.993, **median 1000.509**, maximum
1009.618. That is 64 counts against 64 witness periods, from `p04f-burst` and the console log
`p04f-20261017-091525.log`, and **the medians differ by 0.531 microseconds against a tolerance
of 10**. Criterion 5 holds under the second kernel. The pair shown in the table is the
counter's, 280000 counts at the stated 280 MHz on a 1 kHz tick, for the reason the Zephyr row
gives above; the witness found six bursts in the recording and their median periods span 1.87
microseconds, so the repeat the witness saw stands in for the run the counter measured.

**The witness's excess over the counter is 0.531 microseconds here and 0.504 under Zephyr**, on
periods of 1000 and 1100 microseconds. Two observations of one chain under two kernels, which
is enough to say the excess belongs to the chain and not to either kernel, and not enough to
say what in the chain it is.

**What the two kernels say about each other, which is only ratios.** Within FreeRTOS the block
on one object costs 1.34 of the yield round trip, the block on several
1.17 of the block on one, and the hand to a queue 1.82
of the yield. Within Zephyr, from the run of 08:20 the same morning, the same three ratios are
0.75, 1.76 and 0.97. **They do not agree, and they
were never expected to:** a Zephyr semaphore wake is one switch and cheaper than a yield round
trip of two, while under FreeRTOS the giver's path through the port makes the wake the dearer of
the two. The one ratio that does agree, inheritance off against on, is 4.35 here
and 4.36 under Zephyr, and that agreement is mostly arithmetic rather than evidence
about either kernel: the medium task's work is four times the holder's by construction, so a
ratio near five minus the overhead's share is what the design predicts for any kernel that
inherits. What it does show is that both inherit, which is criterion 4 twice.

## The other eight

The eight are the four bracketed operations under Zephyr, warm and cold, from the run of **Friday
9 October 2026 at 09:40**. Nothing from any earlier run survives in this table, and the reason is
worth the sentence: **every figure this project took before 09:40 that day was substantially a
measurement of its own stopwatch.** `k_cycle_get_32()` was replaced by the timing API that
Zephyr's own `latency_measure` suite uses, and the readings fell by factors of 1.1 to 3.8. The
old figures and what they were is in [BENCH.md](BENCH.md); what forced the change is in
[CRITERION2.md](CRITERION2.md).

Four rows had been entered from the 08:03 run and two operations were held back then, because
their cold value crossed the witness's ten microsecond resolution while their `Scale` said
`below`. **With a credible instrument nothing crosses it**: the eight figures run from 1.79 to
5.64 microseconds, the `below` prediction holds for every one, and the open question about what
the `Scale` column claims does not have to be answered for these rows.

**These are the figures with hardware stack protection on, and that is a decision rather than an
accident of which run came last.** A later run the same evening, at 21:15, turned the guard off
to match Zephyr's own benchmark configuration and met criterion 2 with it; every figure fell, by
about 178 counts per context switch, which is what the guard costs. Those smaller figures are in
[CRITERION2.md](CRITERION2.md) and are **not** the ones to quote: the board's own defconfig turns
the guard on deliberately, real firmware on it runs that way, and the setting went back to the
default the same evening so that a reader building this tree reproduces this table and not the
comparison. The 21:15 numbers describe a configuration nobody should copy into a product.

**The contended mutex rows are relabelled `above`, on Saturday 17 October 2026, and this is the
loud version of a relabel.** They measured 8381 and 36610 counts at 09:40 on Friday 9 October
2026, **29.9 and 130.8 microseconds**, both far above the witness's ten, while their rows said
`below`. The prediction was wrong for a reason that has nothing to do with the instrument and
was visible before any run: the bracket contains, by construction, a holder spinning two
thousand iterations and in one arm a medium thread spinning eight thousand, so the row was never
going to be a few hundred counts. The figures stood in this page's prose from Friday 9 October
2026 and the label moved eight days later, after the FreeRTOS pair arrived at 57.4 and
249.6 and made the prediction wrong under two kernels. The git history shows that order.
The rows keep the cycle counter as their instrument, since a thread's wait is still not a pin
the witness can watch, and they keep their warm and cold rows.

**Their cold halves arrived at 10:09 the same day, and the pair in the table is one run's.** The
campaign's cold pass covered four operations because a comment written before any contended
figure existed said these brackets take long enough to lose a cache state. That is right about
the dilution and wrong about what to do with it, so the pass now covers six; only the period
row, which brackets a millisecond of sleep, stays out. The warm and cold halves of these four
rows are therefore both from the run of 10:09, where every other row's pair is from its own
earlier run, and the two warm figures moved by less than a tenth of a per cent in the process:
8381 to 8379 under Zephyr and 16060 to 16054 under FreeRTOS.

**The penalty is a one-time refill of the code path, and the figures say so.** Under Zephyr it
is 1067 counts with inheritance on and 933 with it off; under FreeRTOS 2496 and 2274. The two arms of
each kernel differ by about a tenth while **the operations themselves differ by a factor of
4.4**, so the penalty plainly does not scale with the span. That was the prediction's refutable
half, written before the run, and it held under both kernels. What it means for a reader of this
table is that the cold column on these four rows is not a cold version of a contended lock: it
is the same lock with one cache refill in front of it, which is why it reads as 2.5 to 15.5 per
cent where the short rows read 57 to 172. The percentages across rows are not comparable and
the counts are.

This file is checked by [scripts/check_instruments.py](../../../scripts/check_instruments.py)
on every push, and the check is **criterion 7**: no row may claim an instrument that cannot see
what the row claims.

## How to read the columns

**Scale** is a prediction, made before any number exists, and that is what makes it checkable.
`below` means the quantity is expected to be shorter than the witness can resolve, so only the
processor's own cycle counter may measure it. `above` means it is long enough for the external
witness. **A measured value that contradicts its own Scale column fails the check**, so a row
cannot be quietly relabelled to suit a number that came out wrong.

Two refinements on Saturday 17 October 2026, after the table had refused six measured figures
and the refusals had been read. **The prediction is held against the warm figure**, the
operation's floor: the column predicted which instrument could see the operation and never
predicted the cache penalty, so a cold figure that crosses ten microseconds is the floor plus a
penalty rather than a prediction that failed. And **a prediction that was wrong is changed
loudly or not at all**: the new label, the date and the reason go beside the table, and the git
history shows the figures standing in prose before the label moved. The check cannot read the
prose; it can see that no `below` row carries a warm figure the witness could have resolved.

**Cache** is warm or cold, and every row the cycle counter measures exists twice; a witness row
states `n/a`, since the witness cannot see a cache. A number taken with the
instruction cache warm and one taken cold differ by enough that a table reporting one of them
alone would be reporting an accident of ordering.

That sentence used to read "differ by more than the quantity being measured in some cases",
written before any cold figure existed. **It was an overstatement and is corrected here rather
than quietly**, and the correction has itself been corrected once, which is recorded rather than
tidied away. On the 08:03 figures the largest cold penalty was 1004 counts and 78 per cent. On the
09:40 figures, taken with a credible instrument, **the largest is 729 counts and the largest
proportion is 105 per cent.**

So a cold reading can now exceed twice its warm one, and the original sentence was nearer the mark
than the correction was. **It is still not reinstated, because it was not right either:** it said
the difference exceeds the quantity "in some cases" on no evidence at all, and one of four cases
at 105 per cent is a thin basis for a general claim. The honest statement is the measured one.

**The penalty is better read in counts than as a proportion**, and the four come out at 380, 525,
703 and 729, a band whose widest is 1.92 times its narrowest. The proportions over the same four
run from 54 to 105 per cent only because each divides by a different baseline.

**Instrument here means which of two instruments may see the row at all**, the processor's own
counter or the external witness, and that is criterion 7's axis. It is a different question from
which *on-device* counter took a reading, which a capture states on its own `instrument` line as
`core` or `wall`: six rows bracket no idle and use the counter that stops in idle, the period
brackets a sleep and uses the one that does not. Both axes exist because a number can be wrong
along either, and the two words are kept apart on purpose.

**Cycles** and **Duration** move together. A cycle count without a duration is not reportable,
because a count is not a time until a clock rate is stated, and the check enforces that pairing.

| Operation | Kernel | Scale | Instrument | Cache | Cycles | Duration us |
|---|---|---|---|---|---|---|
| yield to an equal-priority ready thread | zephyr | below | cycle counter | warm | 691 | 2.47 |
| yield to an equal-priority ready thread | zephyr | below | cycle counter | cold | 1071 | 3.83 |
| yield to an equal-priority ready thread | freertos | below | cycle counter | warm | 626 | 2.24 |
| yield to an equal-priority ready thread | freertos | below | cycle counter | cold | 1000 | 3.57 |
| block on one object, be signalled, return | zephyr | below | cycle counter | warm | 500 | 1.79 |
| block on one object, be signalled, return | zephyr | below | cycle counter | cold | 1025 | 3.66 |
| block on one object, be signalled, return | freertos | below | cycle counter | warm | 840 | 3.00 |
| block on one object, be signalled, return | freertos | below | cycle counter | cold | 2144 | 7.66 |
| block on several objects, be signalled, return | zephyr | below | cycle counter | warm | 850 | 3.04 |
| block on several objects, be signalled, return | zephyr | below | cycle counter | cold | 1579 | 5.64 |
| block on several objects, be signalled, return | freertos | below | cycle counter | warm | 986 | 3.52 |
| block on several objects, be signalled, return | freertos | below | cycle counter | cold | 2316 | 8.27 |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | warm | 677 | 2.42 |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | cold | 1380 | 4.93 |
| hand work to a queue rather than do it in place | freertos | below | cycle counter | warm | 1142 | 4.08 |
| hand work to a queue rather than do it in place | freertos | below | cycle counter | cold | 3144 | 11.23 |
| contended mutex, priority inheritance on | zephyr | above | cycle counter | warm | 8379 | 29.93 |
| contended mutex, priority inheritance on | zephyr | above | cycle counter | cold | 9446 | 33.74 |
| contended mutex, priority inheritance on | freertos | above | cycle counter | warm | 16054 | 57.34 |
| contended mutex, priority inheritance on | freertos | above | cycle counter | cold | 18550 | 66.25 |
| contended mutex, priority inheritance off | zephyr | above | cycle counter | warm | 36628 | 130.81 |
| contended mutex, priority inheritance off | zephyr | above | cycle counter | cold | 37561 | 134.15 |
| contended mutex, priority inheritance off | freertos | above | cycle counter | warm | 69890 | 249.61 |
| contended mutex, priority inheritance off | freertos | above | cycle counter | cold | 72164 | 257.73 |
| periodic thread period | zephyr | above | witness | n/a | 308000 | 1100.00 |
| periodic thread period | freertos | above | witness | n/a | 280000 | 1000.00 |

*Table. Twenty-six rows, all measured, from five runs and each row says which. Zephyr: eight at
09:40 on Friday 9 October 2026, the period at 23:16 that day, the four contended mutex cells at
10:09 on Saturday 17 October 2026. FreeRTOS: six at 08:48 on Saturday 17 October 2026, the
period at 09:27, the four contended mutex cells at 10:09. Every bracketed row is measured with
the processor's own cycle counter. The four contended mutex operations carry `above` since
Saturday 17 October 2026, a relabel explained in full above the table. The two kernels are priced against each
other only in the sense that a ratio between primitives within a kernel can be compared with the
same ratio in the other. No row here licenses a claim that one kernel is faster than the other: that
would be comparing two configurations, two sets of defaults and two compilers' inlining
decisions.*

## What a filled row will have to survive

- the Scale prediction, against the measured warm duration
- the pairing of a cycle count with a duration, so that no count is published without its rate
- the warm and cold pair existing for every cycle-counter row, so that neither is reported alone
- criterion 7 itself: a `below` row naming the witness is refused outright
