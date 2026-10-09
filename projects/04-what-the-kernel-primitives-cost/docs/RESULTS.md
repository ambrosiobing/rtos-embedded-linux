# P04 results

**Eight rows of twenty-six are measured, and the other eighteen read `not measured`.** The method
is written and the refutations are written; most of the runs have not happened. A reader should
still treat this mainly as a protocol, which is why the column exists and is filled in with a
refusal rather than left blank.

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

**It does still have to be answered for the contended mutex rows.** Those measured 8381 and 36610
counts, which are **29.9 and 130.8 microseconds**, both far above the witness's ten, while their
rows say `below`. That prediction was falsified by the first contended run and has nothing to do
with the instrument. They stay unmeasured here because the cold pass covers only the first four
operations, so nothing has to be decided today, and the `Scale` column having now mis-predicted
in two different directions is itself an argument about which resolution to pick.

This file is checked by [scripts/check_instruments.py](../../../scripts/check_instruments.py)
on every push, and the check is **criterion 7**: no row may claim an instrument that cannot see
what the row claims.

## How to read the columns

**Scale** is a prediction, made before any number exists, and that is what makes it checkable.
`below` means the quantity is expected to be shorter than the witness can resolve, so only the
processor's own cycle counter may measure it. `above` means it is long enough for the external
witness. **A measured value that contradicts its own Scale column fails the check**, so a row
cannot be quietly relabelled to suit a number that came out wrong.

**Cache** is warm or cold, and every `below` row exists twice. A number taken with the
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

**Cycles** and **Duration** move together. A cycle count without a duration is not reportable,
because a count is not a time until a clock rate is stated, and the check enforces that pairing.

| Operation | Kernel | Scale | Instrument | Cache | Cycles | Duration us |
|---|---|---|---|---|---|---|
| yield to an equal-priority ready thread | zephyr | below | cycle counter | warm | 691 | 2.47 |
| yield to an equal-priority ready thread | zephyr | below | cycle counter | cold | 1071 | 3.83 |
| yield to an equal-priority ready thread | freertos | below | cycle counter | warm | not measured | not measured |
| yield to an equal-priority ready thread | freertos | below | cycle counter | cold | not measured | not measured |
| block on one object, be signalled, return | zephyr | below | cycle counter | warm | 500 | 1.79 |
| block on one object, be signalled, return | zephyr | below | cycle counter | cold | 1025 | 3.66 |
| block on one object, be signalled, return | freertos | below | cycle counter | warm | not measured | not measured |
| block on one object, be signalled, return | freertos | below | cycle counter | cold | not measured | not measured |
| block on several objects, be signalled, return | zephyr | below | cycle counter | warm | 850 | 3.04 |
| block on several objects, be signalled, return | zephyr | below | cycle counter | cold | 1579 | 5.64 |
| block on several objects, be signalled, return | freertos | below | cycle counter | warm | not measured | not measured |
| block on several objects, be signalled, return | freertos | below | cycle counter | cold | not measured | not measured |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | warm | 677 | 2.42 |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | cold | 1380 | 4.93 |
| hand work to a queue rather than do it in place | freertos | below | cycle counter | warm | not measured | not measured |
| hand work to a queue rather than do it in place | freertos | below | cycle counter | cold | not measured | not measured |
| contended mutex, priority inheritance on | zephyr | below | cycle counter | warm | not measured | not measured |
| contended mutex, priority inheritance on | zephyr | below | cycle counter | cold | not measured | not measured |
| contended mutex, priority inheritance on | freertos | below | cycle counter | warm | not measured | not measured |
| contended mutex, priority inheritance on | freertos | below | cycle counter | cold | not measured | not measured |
| contended mutex, priority inheritance off | zephyr | below | cycle counter | warm | not measured | not measured |
| contended mutex, priority inheritance off | zephyr | below | cycle counter | cold | not measured | not measured |
| contended mutex, priority inheritance off | freertos | below | cycle counter | warm | not measured | not measured |
| contended mutex, priority inheritance off | freertos | below | cycle counter | cold | not measured | not measured |
| periodic thread period | zephyr | above | witness | n/a | not measured | not measured |
| periodic thread period | freertos | above | witness | n/a | not measured | not measured |

*Table. Twenty-six rows, eight of them measured, all from the run of Friday 9 October 2026 at
09:40 and all with the timing API as the instrument. The two kernels are priced against each
other only in the sense that a ratio between primitives within a kernel can be compared with the
same ratio in the other. No row here licenses a claim that one kernel is faster than the other: that
would be comparing two configurations, two sets of defaults and two compilers' inlining
decisions.*

## What a filled row will have to survive

- the Scale prediction, against the measured duration
- the pairing of a cycle count with a duration, so that no count is published without its rate
- the warm and cold pair existing, so that neither is reported alone
- criterion 7 itself: a `below` row naming the witness is refused outright
