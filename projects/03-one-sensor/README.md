# P03. One sensor, two buses, zero code changes

Status on Tuesday 6 October 2026: **the chapter's headline claim is settled, and nothing has
run on the board.** Two cross builds for `nucleo_h7a3zi_q`, made in WSL on the demo laptop
with the Zephyr SDK installed the same day, compile the same application for a two-wire bus
and a four-wire bus and produce **identical instructions**.

The design page is [docs/DESIGN.md](docs/DESIGN.md) and it was committed before any code,
which is the requirement every project here starts with.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q with an ADXL345 breakout, on jumper leads |
| Parts | DFRobot SEN0032, female to female leads, nothing soldered |
| Difficulty | 3 of 5 |

## What was settled, and how

    the instructions are identical: 105 lines of disassembly, no difference
    the objects name the device by its devicetree ordinal, and only that differs: 147 against 160
    one application, two buses, the same code

**The chapter asked for byte for byte identical objects and that is refuted.** The objects
differ at byte 994. The chapter also asked that any difference name what causes it, and it
did: the name of one undefined symbol, `__device_dts_ord_147` against
`__device_dts_ord_160`, carrying the device's ordinal in the generated devicetree. The two
descriptions number the device differently. The compiled instructions are the same.

Nothing in [src/main.c](src/main.c) chooses that ordinal or can see one. The reference is
resolved at link time from whichever description was used, which is the mechanism working
rather than failing, so the criterion was restated rather than patched around:

> the application's instructions are identical between the two builds, and the object
> differs only in the devicetree ordinal of the one device it names

Weaker in letter, stronger in substance. Byte equality could be satisfied by an application
that reads nothing, where this cannot, because the object must still name a device. And byte
equality would have broken the first time anyone added an unrelated node to an overlay,
because that moves every ordinal after it.

## The check was then made to fail, because one that cannot has shown nothing

[scripts/check_object_identity.py](../../scripts/check_object_identity.py) compares the
disassembly and requires the symbol tables to differ in nothing but one ordinal. A scratch
copy of the application was given a real dependency on the bus, printing the device's `reg`
value, and both builds were remade. The check reported:

    FAIL the instructions differ, so the application depends on the bus
        only in build-mut-i2c:   movs    r2, #83 @ 0x53
        only in build-mut-spi:   movs    r2, #0

It names the instruction, not a byte offset. `0x53` is the two-wire address and `0` is the
four-wire chip select index, which is exactly the shape of the defect the criterion exists
to catch: an address became a wire, and an application that read the address compiled
differently because of it.

## What exists

| Part | State |
|---|---|
| [docs/DESIGN.md](docs/DESIGN.md) | the claim, the layering, and the six criteria sorted by what each costs |
| [src/main.c](src/main.c) | the whole application. No `i2c.h`, no `spi.h`, no address, no pin, no `#ifdef` |
| [overlays/i2c.overlay](overlays/i2c.overlay) | the part on the two-wire bus at 0x53 |
| [overlays/spi.overlay](overlays/spi.overlay) | the same part on the four-wire bus, where the address becomes a chip select |
| [overlays/impossible.overlay](overlays/impossible.overlay) | a description that cannot be true, which the build must refuse |
| the four remaining criteria | **need the board** |

## The first real figures this volume has for the target

Both projects before this one build for `native_sim` with the host compiler, which says
nothing about a cross build. These are from `arm-zephyr-eabi-gcc` 14.3.0 against the actual
part.

| Build | Flash of 2 MB | RAM of 256 KB |
|---|---|---|
| two-wire | 31484 B, 1.50 per cent | 7680 B, 2.93 per cent |
| four-wire | 35820 B, 1.71 per cent | 7872 B, 3.00 per cent |

The 4336-byte difference is the four-wire driver costing more than the two-wire one. That is
about the drivers and not about the application, which is the point of the comparison above
being made on the application object alone.

## What is not here

**Nothing has run on hardware.** Criteria 2, 3, 5 and 6 need the board, the breakout and
jumper leads, and the second device criterion 6 wants is not wired. The two criteria that
could be settled without any of that have been.

**No measurement.** Every number on this page is a size or a count reported by a build. The
chapter's timing figures stay unwritten until something is measured with an instrument that
is named.
