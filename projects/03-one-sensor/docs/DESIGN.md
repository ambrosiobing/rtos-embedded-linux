# P03 design: the bus is a description, not a code path

Written on Tuesday 6 October 2026, **before any code in this project exists**, which is the
requirement every project in this volume starts with and the reason the git history is the
evidence rather than this sentence. Every statement below about a build or a reading is a
requirement on work not yet done.

## The claim, and why it is worth a chapter

**One C file, unchanged, reads an accelerometer over a two-wire bus and over a four-wire
bus. The only difference is which text file the build is told to use.** Nobody edits a
driver, nobody recompiles against a different header, and nobody writes an `if` about which
bus is fitted.

That is infrastructure rather than behaviour, which is why it sits after
[P01](../../01-presence/) and [P02](../../02-the-claim/): those two show the device deciding
something, and this one shows how the rest of the volume stays readable when the wiring
changes. The alternative is familiar and is what this design exists to refuse: a build flag
selecting a bus, two code paths that drift, a third board unlike either, and finally an
application nobody can read without knowing the wiring.

## What does the work, and what the application is allowed to know

| Layer | Knows | Does not know |
|---|---|---|
| the overlay | the pins, the bus, the address, the part | anything about what the reading is for |
| the driver | the part's registers and both its transports | which bus this board fitted |
| the application | that there is a device aliased `motion` and that it reports acceleration | the bus, the pins, the address, the part |

The application's entire knowledge of the hardware is one devicetree alias. That is the
mechanism, and the test of whether it holds is not a code review: it is that **the compiled
application object is byte for byte identical between the two builds**. An object that
differs has a dependency on the bus somewhere, and the difference names where.

## The two descriptions

Both hang the same part off a different controller. Neither mentions the application and
the application mentions neither.

    overlays/i2c.overlay    &i2c1 enabled, pinctrl for SCL and SDA, adxl345@53
    overlays/spi.overlay    &spi1 enabled, pinctrl for SCK, MISO, MOSI, adxl345@0

    both end with:  / { aliases { motion = &accel; }; };

The board's own description enables neither bus. That is the first thing to establish rather
than the first thing to work around: the nodes exist and are disabled, and the overlay's job
is to turn one on, give it pins and hang a device off it.

**The pin groups come from the board manual and from RM0455**, which is this part's
reference manual and not the one most published material for an STM32H7 is written against.
That distinction has already cost this bench time elsewhere and is written here so it costs
none again.

## The six criteria, and what each actually needs

This is the part worth settling before any code, because it decides what can be finished
when. Chapter 03 lists six acceptance criteria. They do not all need the same things.

| | Criterion | Needs |
|---|---|---|
| 1 | the application object is byte for byte identical between the two builds | **a cross build only** |
| 2 | both builds read the same at rest, roughly one unit on one axis | the board and the part |
| 3 | a wrong address is reported, not tolerated: it logs a device that is not ready rather than printing zeroes | the board and the part |
| 4 | an overlay naming a bus the board does not have fails at build time, with a message naming it | **a cross build only** |
| 5 | the trigger build samples at the sensor's rate, not the loop's | the board and the part |
| 6 | two devices on one bus do not interfere | the board, the part and a second device |

**Criteria 1 and 4 need no board at all.** One compares two object files and the other
requires a build to fail. Both are compile-time facts, and criterion 1 is the chapter's
headline claim: the strongest thing this chapter says can be demonstrated without wiring
anything.

What they do need is a cross build for `nucleo_h7a3zi_q`, which needs the **Zephyr SDK**.
`native_sim` builds with the host compiler and no SDK, which is why
[P01's Zephyr adapter](../../01-presence/zephyr/) and
[P02's](../../02-the-claim/zephyr/) cost nothing to set up, and it is also why neither of
them proves anything about a cross build. The SDK is not installed on the demo laptop as of
Tuesday 6 October 2026.

So the order of work is: the SDK, then criteria 1 and 4, then the bench for the other four.
That is not the order the chapter lists them in, and the chapter is not wrong; the criteria
are listed by what they mean and this table sorts them by what they cost.

## What this design does not claim

**Portability across two boards.** This chapter proves one application across two buses on
one board. A second board has different peripherals rather than a different wire, which is a
harder claim and belongs to P14.

**That an emulated bus could stand in.** It was worth asking, because an emulator would have
reached criteria 1 and 4 with no hardware at all. **It was asked of the tree on Tuesday 6
October 2026 and the answer is no**: `zephyr/drivers/sensor/adi/adxl345/` carries
`adxl345.c`, its decoder, its RTIO and streaming paths and `adxl345_trigger.c`, and nothing
matching an emulator for this part exists anywhere in the tree. So that route is closed.

Stated precisely, because the loose version of it is wrong: the SDK was always needed for all
six, since nothing is flashed that was not first cross built. What an emulator would have
done is let criteria 1 and 4 be reached **before** the SDK, on `native_sim`, with the host
compiler. It cannot, so the SDK is the first thing on the path and nothing in this project
starts before it.

The driver having a trigger file of its own is the other half of that answer, and it is the
good half: criterion 5 asks that the trigger build sample at the sensor's rate rather than
the loop's, and there is a driver path for it to use rather than an interrupt this project
would have to wire by hand.

**Anything about the second device.** It is dotted in the chapter and absent here until it
has been read on the same bus as the accelerometer.

## The parts, which are all on the bench

| Part | Role |
|---|---|
| NUCLEO-H7A3ZI-Q | the host for every build |
| DFRobot SEN0032 | an ADXL345 breakout that speaks both buses, which is what makes it the right part for this chapter |
| female jumper leads | every header here is female, so the leads are female to female and nothing is soldered |

No shield is fitted, deliberately. A shield would hide the wiring behind a connector, and the
wiring is the subject.
