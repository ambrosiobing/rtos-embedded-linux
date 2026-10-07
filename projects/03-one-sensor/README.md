# P03. One sensor, two buses, zero code changes

Status on Wednesday 7 October 2026: **both criteria that need no hardware are settled, and the
two-wire half of criterion 2 has now been measured on the board.** Two cross builds for `nucleo_h7a3zi_q`, made in WSL on the
demo laptop with the Zephyr SDK installed the same day, compile the same application for a
two-wire bus and a four-wire bus and produce **identical instructions**. A third build, of a
description that cannot be true, is refused.

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

## Criterion 4: a description that cannot be true is refused

An overlay naming a bus the board does not have must fail at build time with a message
naming it. [overlays/impossible.overlay](overlays/impossible.overlay) references `&i2c5`;
this part has `i2c1` through `i2c4`. The build says:

    devicetree error: ./overlays/impossible.overlay:24 (column 1):
        parse error: undefined node label 'i2c5'

and CMake then reports that configuring is incomplete and stops. It names the bus. That is the difference between a configuration file and a checked one: a
description merely wrong about a pin gives a board that does not work and a long afternoon
finding out why, where a description naming a controller that does not exist does not
survive the build at all.

## What exists

| Part | State |
|---|---|
| [docs/DESIGN.md](docs/DESIGN.md) | the claim, the layering, and the six criteria sorted by what each costs |
| [src/main.c](src/main.c) | the whole application. No `i2c.h`, no `spi.h`, no address, no pin, no `#ifdef` |
| [overlays/i2c.overlay](overlays/i2c.overlay) | the part on the two-wire bus at 0x53 |
| [overlays/spi.overlay](overlays/spi.overlay) | the same part on the four-wire bus, where the address becomes a chip select |
| [overlays/impossible.overlay](overlays/impossible.overlay) | a description that cannot be true, which the build must refuse |
| [docs/WIRING.md](docs/WIRING.md) | the two jumpers and four leads of the two-wire build as it is actually wired, from the vendor's schematic, and why the digital pins have no margin |
| [the volume's hardware documents](../../docs/HARDWARE.md) | every datasheet behind this bench, what reading them decided, and three figures drawn rather than reproduced |
| [docs/BENCH.md](docs/BENCH.md) | the first bring-up on the board, the fault it found, and the three bench facts that cost an hour |
| [diag/](diag/) | a bus scan, a separate application, written because one FAIL line named five possible causes |
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

**The four-wire build has never been wired.** Criterion 2 asks that both builds read the same
at rest; the two-wire build now reads 9.88, 9.83 and 9.97 metres per second squared against a
true 9.8066, and the four-wire build has not been connected, so the comparison the criterion
exists to make has not been made. Criteria 3, 5 and 6 still need the board, and the second
device criterion 6 wants is not wired.

**The measurement was taken with the module held by hand.** Its eight-pin header is not
fitted, so all six connections are friction contacts in bare holes, and one run in roughly
fifteen held them closed for the two seconds the application needs.
[docs/BENCH.md](docs/BENCH.md) records both the figure and how it was obtained.

**No measurement.** Every number on this page is a size or a count reported by a build. The
chapter's timing figures stay unwritten until something is measured with an instrument that
is named.

**A note on reading a build's exit status.** The run that settled criterion 4 was piped
through `tail`, so the shell reported the exit status of `tail` and not of `cmake`, which
was zero and meant nothing. The evidence is the `Configuring incomplete` line. The CI job
inverts the build's own status instead, because a pipeline's exit status is the last
command's and that has now caught this project twice: the Zephyr phase runs are gated on a
printed line for the same reason.
