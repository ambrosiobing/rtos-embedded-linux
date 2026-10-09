# P04 results

**Four rows of twenty-six are measured, and the other twenty-two read `not measured`.** The
method is written and the refutation is written; most of the runs have not happened. A reader
should still treat this mainly as a protocol, which is why the column exists and is filled in
with a refusal rather than left blank.

The four filled rows arrived on Friday 9 October 2026 at 08:03, and the reason only four arrived
is itself worth a sentence. **All four bracketed operations were measured warm and cold in that
run and all eight figures are sound.** Two operations are entered here and two are not, because
their cold value crosses the witness's ten microsecond resolution while their `Scale` says
`below`:

Entered, both arms under the boundary: `hand work to a queue` at 4.27 microseconds warm and 7.64
cold, and `block on one object` at 5.22 warm and 8.23 cold. Held back: `block on several objects`
at 6.43 warm and **10.01** cold, over by 0.14 per cent, and the `yield` round trip at 9.25 warm
and **11.84** cold, over by 18 per cent.

**The `below` prediction holds warm for all four and fails cold for two.** That is a falsified
prediction rather than a bad measurement, and this page's own check refuses the rows rather than
letting them be relabelled. The resolution is a change to what the `Scale` column claims, which
is a change to the meaning of a published check and not a repair, so it is written up in
[BENCH.md](BENCH.md) and left for a decision rather than taken quietly here.

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
than quietly.** Measured on Friday 9 October 2026, the largest cold penalty on this board is 1004
counts and the largest as a proportion is 78 per cent, on the cheapest operation. Neither is more
than the quantity. The point survives at three quarters as well as it would at two, and the
figure did not need inflating.

**The penalty is better read in counts than as a proportion**, and the four measured come out at
725, 843, 944 and 1004, a band whose widest is 1.39 times its narrowest. The proportions over the
same four run from 27 to 78 per cent only because each divides by a different baseline.

**Cycles** and **Duration** move together. A cycle count without a duration is not reportable,
because a count is not a time until a clock rate is stated, and the check enforces that pairing.

| Operation | Kernel | Scale | Instrument | Cache | Cycles | Duration us |
|---|---|---|---|---|---|---|
| yield to an equal-priority ready thread | zephyr | below | cycle counter | warm | not measured | not measured |
| yield to an equal-priority ready thread | zephyr | below | cycle counter | cold | not measured | not measured |
| yield to an equal-priority ready thread | freertos | below | cycle counter | warm | not measured | not measured |
| yield to an equal-priority ready thread | freertos | below | cycle counter | cold | not measured | not measured |
| block on one object, be signalled, return | zephyr | below | cycle counter | warm | 1461 | 5.22 |
| block on one object, be signalled, return | zephyr | below | cycle counter | cold | 2304 | 8.23 |
| block on one object, be signalled, return | freertos | below | cycle counter | warm | not measured | not measured |
| block on one object, be signalled, return | freertos | below | cycle counter | cold | not measured | not measured |
| block on several objects, be signalled, return | zephyr | below | cycle counter | warm | not measured | not measured |
| block on several objects, be signalled, return | zephyr | below | cycle counter | cold | not measured | not measured |
| block on several objects, be signalled, return | freertos | below | cycle counter | warm | not measured | not measured |
| block on several objects, be signalled, return | freertos | below | cycle counter | cold | not measured | not measured |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | warm | 1196 | 4.27 |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | cold | 2140 | 7.64 |
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

*Table. Twenty-six rows, four of them measured. The two kernels are priced against each other
only in the sense that a ratio between primitives within a kernel can be compared with the same
ratio in the other. No row here licenses a claim that one kernel is faster than the other: that
would be comparing two configurations, two sets of defaults and two compilers' inlining
decisions.*

## What a filled row will have to survive

- the Scale prediction, against the measured duration
- the pairing of a cycle count with a duration, so that no count is published without its rate
- the warm and cold pair existing, so that neither is reported alone
- criterion 7 itself: a `below` row naming the witness is refused outright
