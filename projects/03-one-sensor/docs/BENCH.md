# First bring-up on the board, Tuesday 6 October 2026

What happened the first time this project met hardware, written while it was happening rather
than afterwards. **Nothing here settles a criterion.** It is the account of a fault, and the
reason it is in the repository is that most of the evening was spent eliminating things that
turned out to be correct, which is the part nobody writes down.

## The fault

**The SEN0032 had no supply.** Only PB8, PB9 and GND ran from the NUCLEO-H7A3ZI-Q to the
breadboard. There was no lead from the board's `+3V3` to the module's VCC row.

That single omission produced two faults at once, which is why it was hard to see. The module
had no power, and **jumper B had nothing to put on CS**, so even a powered part would have
stayed in four-wire mode and ignored the two-wire bus. One missing lead, two reasons for
silence.

## How it was fixed: the supply does not go through the breadboard rail

The module's VCC is now wired **straight to the NUCLEO-H7A3ZI-Q's `+3V3` pin**, rather than
to the breadboard's power rail and from there to the module.

**That improved it and did not end it, and the first version of this paragraph said it did.**
The claim was written from one report before any run tested it, which is the mistake this
volume has a standing rule against. The runs afterwards: the application failed three times
out of three at `FAIL adxl345@53 is not ready`, and the scan found `0x53` in two runs out of
three. In one of those the part answered its address and returned `DEVID 0xe5`, then failed
the very next register read a few milliseconds later. The shield's own `0x19` dropped out of
one scan and a spurious `answer at 0x0c` appeared in another, neither of which belongs to any
part on this bench.

So the rail was **a** fault rather than **the** fault. What remains is the friction contacts,
and the timing makes the shape of it clear: a scan needs a few milliseconds of contact and
the application needs about two continuous seconds, because it checks the device, then takes
ten readings two hundred milliseconds apart. **A scan that succeeds is not evidence that the
application will.**

**A power rail is the one conductor the continuity tests in this log cannot check.** Tying a
signal row to the GND row and watching the bus die proves the signal lead and the ground lead,
because both are in the path. The supply rail is in none of those paths, so it was the only
part of the circuit that was never tested, and it was the part that was open.

Worth carrying to every bench session here: take a supply from the host's pin to the part,
and let the breadboard carry signals and the links that set a part's mode.

## The fact that reframes all of it: nothing on this module is soldered

**The SEN0032's eight-pin header is not fitted. The module has bare plated holes**, and every
connection to it is a friction contact: a male pin pushed into a hole and held against the
wall of that hole by tilting the board, with rolled plastic holding the tilt. That is how it
was read successfully on a Raspberry Pi 3 Model B+, and it is the only way it has ever been
read.

So "the six connections are in place" means they are **positioned**, not that they conduct,
and any one of them can be open at any moment including one that was closed a minute earlier.
There is no soldering iron on this bench, so fitting a header is not available. The
arrangement that does not depend on a held tilt is to push each male pin through the module's
hole and down into a breadboard, so the breadboard's own spring grips the pin and presses it
against the hole wall.

This belongs at the top of any future bring-up of this part, ahead of every test below it.

## What the application said, and why that was not enough

The first run on the board printed

    *** Booting Zephyr OS build v4.5.0-rc1-170-g8f62a4ab82b5 ***
    one sensor, on whichever bus the overlay described

      FAIL adxl345@53 is not ready, so nothing below would be a reading

which is [src/main.c](../src/main.c)'s guard working exactly as criterion 3 asks: it refused
to print zeroes from a part it never reached. It is also the least informative true statement
available. Five faults produce that line and it distinguishes none of them.

That is what [diag/](../diag/) was written for, and the scan it prints is what turned the rest
of the evening from guessing into reading.

## Three facts about this bench that cost an hour between them

**The ST-LINK virtual COM port stays silent until DTR and RTS are raised.** Three console runs
returned zero bytes and were nearly put down to the wiring. A `SerialPort` opened with its
defaults never delivers anything on COM13; setting `DtrEnable` and `RtsEnable` before `Open`
delivers everything, including output buffered from a previous boot.

**An application that prints and returns is over in under a second.** The console has to be
open first and the board reset while it is listening. Unplugging the USB cable resets the
ST-LINK as well, which discards whatever it had buffered, so a reader opened after a power
cycle sees nothing even when everything works.

**Drag and drop flashing is silent by design, and its silence is ambiguous.** A `.bin` copied
to the drive disappears whether or not it was programmed, and the drive's idle state is
`DETAILS.TXT` and `MBED.HTM` alone. A vanished file and no `FAIL.TXT` therefore looks
identical to success and to several kinds of failure. `STM32_Programmer_CLI.exe`, bundled with
STM32CubeIDE, reports what it did.

## The control, which is what cleared the board

The X-NUCLEO-IKS5A1 runs its two-wire bus on Arduino D14 and D15, which are PB9 and PB8, the
identical pins this project uses, and it carries its own pull-up resistors. Stacking it and
running the **same binary** produced

    scanning 0x08 to 0x77
      answer at 0x19
      answer at 0x1e
      answer at 0x5c
      answer at 0x6a
      answer at 0x6b
    5 address(es) answered

three times in a row. That cleared the controller, the pin assignment, the pinctrl, the clock,
the overlay and the scan software in one step, and moved the whole question to the breadboard.

A control that shares the subject's pins and differs only in what hangs off them is worth more
than any amount of re-reading the description.

## The bus as a continuity tester

There is no multimeter on this bench. There is a two-wire bus, and holding one of its lines
low is a normal and safe condition, because every line is open drain. So the bus can be asked
whether a lead conducts.

Linking a lead's breadboard row to the GND row and running the scan gives two distinct
signatures, and both are readings:

| What was held | What the scan did | What it proved |
|---|---|---|
| SDA, through lead 4 | `0 address(es) answered` | PB9 reaches that row |
| SCL, through lead 3 | printed `scanning 0x08 to 0x77` and stopped | PB8 reaches that row |

The second is the more useful of the two. Holding the clock low stalls a transfer rather than
failing it, so the scan never returns and the output stops mid-line. **A hang is therefore a
measurement**, and recognising it later identified an accidental short in one run rather than
several.

This also proved lead 2 along the way: tying SDA to the GND row only stops the bus if that row
is actually at ground.

## What this says about the module, and what it does not

The SEN0032 carries **no resistors at all**. Its complete component list is the ADXL345, the
BL8555-30 regulator, three 104P capacitors, one 4.7 uF capacitor and the header. There is no
pull-up on SDA, none on SCL, and none on CS or SDO, which is why both jumpers are required and
why the only pull-ups in the circuit are the processor's own, around 40 kOhm.

Whether 40 kOhm is enough for this wiring **is still unknown**, and the evening did not answer
it. The module was unpowered during every run, including the ones where the shield's 4.7 kOhm
resistors were on the same bus, so no comparison was ever made between the two pull-up
strengths. That question is open and belongs to the next session.

## The measurement, in the small hours of Wednesday 7 October 2026

One run in roughly fifteen attempts reached `10 readings taken`, with the module held down by
hand so that its six friction contacts stayed closed for the two seconds the application
needs. **How it was obtained is part of the result**, in the same way an instrument is named.

Its first four readings, converted back to counts at `SENSOR_G / 64` per count:

| Reading | x | y | z | vector, counts | vector, m/s squared |
|---|---|---|---|---|---|
| 1 | -30 | 20 | 48 | 60.03 | 9.20 |
| 2 | -33 | 19 | 52 | 64.45 | **9.88** |
| 3 | -33 | 18 | 52 | 64.16 | **9.83** |
| 4 | -38 | 22 | 48 | 65.05 | **9.97** |

One g is **64 counts** in this mode, which is 9.8066 metres per second squared. Readings 2, 3
and 4 sit inside two per cent of that, and a single count is already 1.6 per cent of a g at
the plus and minus eight g range. Readings 5 to 10 are the hand: line 5 is one count on every
axis, which is a dropped read, and 6 to 8 are a real acceleration from the pressure.

**That settles the two-wire half of criterion 2 and only that half.** The criterion asks that
BOTH builds read the same at rest, and the four-wire build has never been wired, so the
comparison it exists to make has not been made.

**It also withdraws the scale problem.** The 8.35 g measured before the supply was corrected
was the wandering supply and not a driver defect, so the hypothesis that the driver ignores
the full resolution bit when scaling is **not** supported by anything here. The driver's
sensitivity table, `SENSOR_G / 64` for the eight g range, is correct, and the range is eight g
because `adi,adxl345-common.yaml` gives `range` a default of 2 and the overlay names none.

One consequence worth recording for later rather than acting on now. At plus and minus eight g
the quantisation is 15.6 mg, where this application never sees more than one g. Setting
`range = <ADXL345_DT_RANGE_2G>` in the overlay would give four times the resolution, and it
would also change the devicetree, so criterion 1's object comparison would have to be run
again afterwards rather than assumed.

## One mistake worth keeping

A spare jumper added for a continuity test was left in place after the test became
unnecessary, and when a supply finally reached the VCC row it connected `+3V3` directly to
SDA. The bus hung, which is exactly the signature described above, and it was recognised from
the earlier deliberate test rather than investigated from scratch.

The lesson is about the instructions rather than the wiring: a temporary change needs its
removal written down at the same time it is made, not when it is next remembered.
