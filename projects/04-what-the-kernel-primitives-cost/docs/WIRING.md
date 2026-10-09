# P04 wiring: two leads, both endpoints of each

**Written Friday 9 October 2026, from the firmware's own devicetree and the bench record, and
not from the footage.** [z04_as_built.gif](figures/z04_as_built.gif) in this folder shows two
leads between the NUCLEO-H7A3ZI-Q and the MCC 118, one orange and one dark, and at 640 pixels
across a pan it does not show which is which or where either lands. This page is the table the
footage cannot be. A wiring table is an observation with a date, and the date is above.

This is the only project in the volume whose table has a **second instrument**, and the only
wire P04 will ever need. The six bracketed rows are the processor and the kernel talking to
themselves over one USB cable; the seventh, the periodic thread's period, is the one row long
enough for an external witness to see, and criterion 5 asks that the two instruments agree on it.

## The table

| Lead | Signal | NUCLEO-H7A3ZI-Q end | MCC 118 end | Direction |
|---|---|---|---|---|
| 1 | the marker, **PB4** | **CN7 pin 19**, Zio header, labelled D25 | screw terminal **CH0** | board drives, HAT reads |
| 2 | ground | any pin marked **GND** on the Zio or Arduino headers; the silkscreen is authority for ground | screw terminal **GND**, on the same block as CH0 | reference, no direction |

*Table. Two leads, Friday 9 October 2026. The colours of the two leads in the footage are not
assigned to these rows, because nothing on this page can tell them apart; when they are
confirmed by eye they belong in a third column here, dated.*

**Ground first, before the signal lead, on the way up; signal first on the way down.** And the
Raspberry Pi is powered before the NUCLEO, and the NUCLEO is powered off before the Pi, which is
the sibling volume's order and is kept because a HAT input driven while its own supply is off
is the condition the MCC 118's user guide says to avoid.

## Where each endpoint comes from, so that each can be checked

**PB4** is named once, in [app.overlay](../zephyr/app.overlay):

    marker-gpios = <&gpiob 4 GPIO_ACTIVE_HIGH>;

and is read from there by `GPIO_DT_SPEC_GET` in [main.c](../zephyr/src/main.c). The run prints
what the build handed it:

    #   marker for the witness: pin 4 on gpio@58020400, configured as an output, one toggle per period

`gpio@58020400` is the devicetree node for the port at that register base, which in RM0455 is
**GPIOB**, and the pin is the bit. That is the one translation this page makes, and it is the
line a reader checks the table against.

**CN7 pin 19** is UM2408 Rev 6, the manual for the MB1363 board family, Table 18 on page 44,
"NUCLEO-H7A3ZI-Q pin assignments": CN7 pin 19, signal D25, function SPI_B_MISO on SPI3, remark
column empty. Its labelled function is an alternate function the pin can take and nothing on the
board drives it, which is what "free" has to mean. Table 17 on the pages before it is the H745's
and H755's and carries the identical row for PB4, so the wrong table would have given the right
answer; the caption was checked. The reading was done for the sibling firmware volume on Tuesday
6 October 2026 and is reused here, because the lead was already on that pin and a pin chosen to
suit this firmware would have cost a rewiring for nothing.

**Not on an Arduino alias**, and that was checked rather than assumed: Zephyr's
`arduino_r3_connector.dtsi` for this board maps D0 to D15 and A0 to A5, and `&gpiob 4` is on
none of them. ST's D25 is a Zio label with no Zephyr binding.

**CH0 and GND on the MCC 118** are the terminals the sibling volume's `scan.py` reads and
documents, and the ones the bench record of Friday 9 October 2026 says the leads went to when
the HAT went live. They are on the green screw terminal block nearest the HAT's edge in the
footage. The HAT resolves about ten microseconds per sample at 100 kS/s on one channel, and
its spread floor on a clean signal measured about 3.4 microseconds that morning, so a period
of 1.1 ms is seen comfortably and a context switch of 2 microseconds is not, which is what
criterion 7 and [RESULTS.md](RESULTS.md)'s `Scale` column are about.

## What the marker does, and why it toggles

One edge per period boundary, alternating, never a pulse. The sibling volume tried a pulse of
two consecutive stores, tens of nanoseconds wide, and the 10 microsecond sampler caught
fragments of it up to half a volt in 346 samples of 199784 and reported a rate that was the rate
of the catches. A toggle leaves the level where the boundary put it until the next boundary, so
every crossing in either direction is a boundary and the interval between consecutive edges is
one period. [witness.py](../host/witness.py) counts both polarities.

The firmware toggles in the statement after it reads the wall counter, so the instant the
processor's instrument records and the instant the witness sees are the same instant to within
a store. **64 periods, 65 edges**, and the witness's intervals line up one to one with the
device's `a_counts`. Outside those 70 milliseconds the pin sits still, so most of a recording
that spans the run is a flat line with one burst in it, and `witness.py` refuses a recording
that is flat throughout as a wire not fitted or a pin not driven.

## How the capture reaches the reduction

The MCC 118 is on the Raspberry Pi `eplepi`, which carries the sibling volume's `scan.py` and
`daqhats`, and does not carry this repository. The sibling's capture script is used as it is,
because it already paid for the `daqhats` version mismatch, the overrun handling and the lesson
that a capture is evidence and is never overwritten. **Two of its metadata fields do not apply
to a P04 run and are read by nothing here**: `--build` accepts only that volume's three build
names and is set to `timer` as a placeholder, and `firmware_identity` is a `git describe` of
that repository, not this one. The identity of what actually ran is the build stamp on the P04
console, and the `--note` names this project so the JSON cannot later be read as a P06 capture.

The CSV and JSON travel to the laptop that holds this repository, and there:

    python3 witness.py run.csv run.json > witness.txt
    cat device.txt witness.txt > whole.txt
    python3 reduce.py whole.txt

where `device.txt` is the `periodic thread period` block cut from the console log, from its
`instrument wall` line to the end of its `a_counts` line. `reduce.py` refuses a device half
labelled `core`, refuses a witness half without a sample rate, and reports agreement or
disagreement of the two medians within the witness's own resolution.
