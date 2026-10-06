# A bus scan, because one FAIL line named nothing

Written on Tuesday 6 October 2026, after the project's own application ran on the
NUCLEO-H7A3ZI-Q for the first time and printed

    *** Booting Zephyr OS build v4.5.0-rc1-170-g8f62a4ab82b5 ***
    one sensor, on whichever bus the overlay described

      FAIL adxl345@53 is not ready, so nothing below would be a reading

That line is [src/main.c](../src/main.c)'s guard doing exactly what criterion 3 asks of it:
refusing to print zeroes from a part it never reached. It is also the least informative true
statement available, because five different faults produce it and the line distinguishes none
of them.

| Could have caused it | Would be shown by |
|---|---|
| SDO not held low, so the part is at 0x1d | an answer at 0x1d and silence at 0x53 |
| CS not held high, so the part is in four-wire mode and ignores the two-wire bus | silence at both addresses |
| SCL and SDA swapped between PB8 and PB9 | silence at every address |
| the bus clocked faster than the pull-up resistors support | silence at 400 kHz, an answer at 100 kHz |
| the controller never initialising | the controller reporting itself not ready, before any scan |

**This application exists rather than an edit to the one next door.** The project's
application must not change: its object is the evidence for criterion 1, and the two builds
of it have already been compared instruction by instruction. A diagnostic that edited
`src/main.c` would have invalidated that comparison in order to answer a question about a
breadboard.

This one is allowed to include `i2c.h`, which the application next door is not. Finding out
about the bus is its entire purpose, where the other one's purpose is not knowing there is a
bus at all.

## What it reads out of the description rather than repeating

The address and the clock both come from the devicetree, through `DT_REG_ADDR` and
`DT_PROP`. A diagnostic carrying its own copy of the address would report a healthy bus while
the overlay was wrong, and that is the one outcome that would cost a whole evening.

## The two builds

Both use the project's own [overlays/i2c.overlay](../overlays/i2c.overlay) unchanged. The
second adds [overlays/slow.overlay](overlays/slow.overlay) on top of it, which alters the
clock and nothing else, so the comparison is of one property.

A two-wire bus is open drain: each device pulls a line down and a resistor pulls it back up.
The rise time is that resistor working against the capacitance of the leads, and a 400 kHz bus
allows roughly a fifth of the rise time a 100 kHz bus does. On breadboard jumpers, behind
whatever pull-up the module carries and the processor's own weak internal one, 400 kHz is the
rate most likely to fail. **An answer at 100 kHz and silence at 400 kHz means the rise time,
not the wiring**, and that is a finding worth the chapter rather than a workaround.

## What it is not

**Not a test.** Nothing here passes or fails; it prints what answered. The criteria are in
[../docs/DESIGN.md](../docs/DESIGN.md) and this settles none of them.

**Not in CI.** Every line it prints requires the board.
