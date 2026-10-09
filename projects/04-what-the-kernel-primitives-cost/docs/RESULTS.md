# P04 results

**Two rows of twenty-six are measured, and the other twenty-four read `not measured`.** The
method is written and the refutation is written; most of the runs have not happened. A reader
should still treat this mainly as a protocol, which is why the column exists and is filled in
with a refusal rather than left blank.

The two filled rows arrived on Friday 9 October 2026 and the reason only two arrived is itself
worth a sentence. Four operations were measured warm and cold that day. Two of the four had
their cold arm measured with the invalidation sitting where it could not reach the bracket, so
their cold figures describe the case rather than the kernel. A third, the yield round trip, has
a sound pair whose cold value is 11.85 microseconds, **above the witness's ten microsecond
resolution and therefore refused by this page's own check** while its `Scale` says `below`. One
operation of the four had both arms sound and both under the boundary. See
[BENCH.md](BENCH.md) for all four figures and for the question the yield row leaves open.

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
than quietly.** Measured on Friday 9 October 2026, the largest cold penalty on this board is 912
counts on a 1201 count operation, which is 75 per cent and not more than the quantity. The point
survives at three quarters as well as it would at two; the figure did not need inflating.

**Cycles** and **Duration** move together. A cycle count without a duration is not reportable,
because a count is not a time until a clock rate is stated, and the check enforces that pairing.

| Operation | Kernel | Scale | Instrument | Cache | Cycles | Duration us |
|---|---|---|---|---|---|---|
| yield to an equal-priority ready thread | zephyr | below | cycle counter | warm | not measured | not measured |
| yield to an equal-priority ready thread | zephyr | below | cycle counter | cold | not measured | not measured |
| yield to an equal-priority ready thread | freertos | below | cycle counter | warm | not measured | not measured |
| yield to an equal-priority ready thread | freertos | below | cycle counter | cold | not measured | not measured |
| block on one object, be signalled, return | zephyr | below | cycle counter | warm | not measured | not measured |
| block on one object, be signalled, return | zephyr | below | cycle counter | cold | not measured | not measured |
| block on one object, be signalled, return | freertos | below | cycle counter | warm | not measured | not measured |
| block on one object, be signalled, return | freertos | below | cycle counter | cold | not measured | not measured |
| block on several objects, be signalled, return | zephyr | below | cycle counter | warm | not measured | not measured |
| block on several objects, be signalled, return | zephyr | below | cycle counter | cold | not measured | not measured |
| block on several objects, be signalled, return | freertos | below | cycle counter | warm | not measured | not measured |
| block on several objects, be signalled, return | freertos | below | cycle counter | cold | not measured | not measured |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | warm | 1201 | 4.29 |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | cold | 2113 | 7.55 |
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

*Table. Twenty-six rows, two of them measured. The two kernels are priced against each other
only in the sense that a ratio between primitives within a kernel can be compared with the same
ratio in the other. No row here licenses a claim that one kernel is faster than the other: that
would be comparing two configurations, two sets of defaults and two compilers' inlining
decisions.*

## What a filled row will have to survive

- the Scale prediction, against the measured duration
- the pairing of a cycle count with a duration, so that no count is published without its rate
- the warm and cold pair existing, so that neither is reported alone
- criterion 7 itself: a `below` row naming the witness is refused outright
