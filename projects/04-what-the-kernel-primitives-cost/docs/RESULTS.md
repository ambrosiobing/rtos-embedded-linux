# P04 results

**Every row reads `not measured`.** The method is written and the refutation is written; the
runs have not happened. A reader should treat this as a protocol rather than as a result, which
is why the column exists and is filled in with a refusal rather than left blank.

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
instruction cache warm and one taken cold differ by more than the quantity being measured in
some cases, so a table reporting one of them would be reporting an accident of ordering.

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
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | warm | not measured | not measured |
| hand work to a queue rather than do it in place | zephyr | below | cycle counter | cold | not measured | not measured |
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

*Table. Twenty-two rows, none of them measured. The two kernels are priced against each other
only in the sense that a ratio between primitives within a kernel can be compared with the same
ratio in the other. No row here licenses a claim that one kernel is faster than the other: that
would be comparing two configurations, two sets of defaults and two compilers' inlining
decisions.*

## What a filled row will have to survive

- the Scale prediction, against the measured duration
- the pairing of a cycle count with a duration, so that no count is published without its rate
- the warm and cold pair existing, so that neither is reported alone
- criterion 7 itself: a `below` row naming the witness is refused outright
