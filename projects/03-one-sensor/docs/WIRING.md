# Wiring the SEN0032 to the NUCLEO-H7A3ZI-Q

Written on Tuesday 6 October 2026, from the vendor's own schematic and the Analog Devices
datasheet rather than from another maker's ADXL345 breakout, because the pad order and the
supply arrangement both vary between makers and getting either wrong is expensive.

**Built and checked on Tuesday 6 October 2026.** Nothing is soldered. The board is unplugged
from USB while leads are moved.

## The two-wire wiring, for the part at 0x53

This page first listed six leads from the module to the board. The wiring actually built uses
a breadboard, and on a breadboard two of those six never leave the module's own rows: SDO is
held low and CS is held high by short links to the GND and VCC rows that two of the leads
already feed. Six connections either way. Two of them are local to the module and four reach
the NUCLEO-H7A3ZI-Q.

Two jumpers, each a short link between two rows of the same breadboard:

| Jumper | From | To | What it does |
|---|---|---|---|
| A | SEN0032 **SDO** row | SEN0032 **GND** row | address select held low, which makes the part 0x53 |
| B | SEN0032 **CS** row | SEN0032 **VCC** row | bus select held high, which puts the part in two-wire mode |

Four leads, each from a breadboard row to the NUCLEO-H7A3ZI-Q Morpho header:

| Lead | Colour | SEN0032 pad | NUCLEO-H7A3ZI-Q | Signal and direction |
|---|---|---|---|---|
| 1 | red | 2, **VCC** | **3V3** | supply, board to module |
| 2 | black | 1, **GND** | **GND** | common return |
| 3 | yellow | 8, **SCL** | **PB8**, the Arduino **D15** position | clock, board to module |
| 4 | green | 7, **SDA** | **PB9**, the Arduino **D14** position | data, both directions |

The two tables read as a contradiction unless the rows are set out, because the GND row and
the VCC row each carry a jumper **and** a lead. Row by row, which is the form that cannot be
misread:

| SEN0032 pad | Everything in that breadboard row |
|---|---|
| 1, **GND** | lead 2, black, to a NUCLEO-H7A3ZI-Q pin printed **GND**, and one end of jumper A |
| 2, **VCC** | lead 1, red, to the NUCLEO-H7A3ZI-Q pin printed **3V3**, and one end of jumper B |
| 3, **CS** | the other end of jumper B, and nothing else |
| 4, **INT1** | nothing |
| 5, **INT2** | nothing |
| 6, **SDO** | the other end of jumper A, and nothing else |
| 7, **SDA** | lead 4, green, to **PB9** |
| 8, **SCL** | lead 3, yellow, to **PB8** |

So SEN0032 GND does reach the board's GND, and SEN0032 VCC does reach the board's 3V3. The
jumpers do not reach the board; they borrow rows that already do.

Pads 4 and 5, **INT1** and **INT2**, stay unconnected. The trigger build of criterion 5 will
want one of them; nothing before that does.

**Jumper B is the one checked before power, and the check travels alone.** It puts whatever
lead 1 delivers onto CS, and CS reaches the die with no level shifter and an absolute maximum
of 3.3 V, for the reason set out further down this page. The question asked and answered
before the USB cable went in on Tuesday 6 October 2026 was whether lead 1's female end sits
on the Morpho pin printed `3V3` rather than `5V` or `VIN`. It does.

**The pad order is the silkscreen's, not the schematic's.** Along the eight pads, from the
end opposite the mounting hole toward it, the printing reads GND, VCC, CS, INT1, INT2, SDO,
SDA, SCL, with SCL beside the hole and the axis legend. The schematic's connector J1 numbers
the same nets differently: J1 pin 1 is VCC, pin 2 is CS, and GND is the unlabelled end pin.
Following J1's numbering would put the supply one pad out.

**Why PB8 and PB9.** `boards/st/nucleo_h7a3zi_q/arduino_r3_connector.dtsi` maps
`ARDUINO_HEADER_R3_D15` to `&gpiob 8` and `D14` to `&gpiob 9`, which is where
[overlays/i2c.overlay](../overlays/i2c.overlay) puts SCL and SDA. The board's own file is the
source; the chapter agreed with it, which is pleasant but is not why it is written here.

## The supply, where two vendor pages disagree and both are right

The DFRobot product page says 2.0 to 3.6 V and the wiki says 3.3 to 6 V. Neither is wrong,
and the schematic says why: **U2 is a BL8555-30 regulator** with the header's VCC on its
input and the die on its output. 2.0 to 3.6 V is the ADXL345's own supply range, and 3.3 to
6 V is what the module accepts because the regulator sits between them.

So **3.3 V on VCC**, which is inside the module's range, and the die sees the regulated rail
whatever happens above it. 5 V is what the regulator exists to allow and nothing here needs
it.

## The finding worth recording: the digital pins have no margin

The ADXL345 datasheet's absolute maximum for digital pins is

> -0.3 V to VDD I/O + 0.3 V **or 3.6 V, whichever is less**

The schematic ties the die's VDD I/O, pin 1, to the regulator's output. A
BL8555-**30** outputs 3.0 V, so the digital maximum on this module is **3.3 V**.

Driving SCL, SDA and CS from a 3.3 V host puts them at exactly that. **Permitted, and with
no margin at all.**

The cause is structural rather than something this wiring chose. CS, SDO, SDA and SCL run
from the header straight to the die with no level shifter, which the schematic shows plainly:
the module's advertised 3.3 to 6 V input range applies to **VCC alone**. A 5 V host on these
four signals would be well over the limit, and a 3.3 V host is exactly on it. There is no
supply arrangement that improves this, because powering VCC higher does not move
VDD I/O: the regulator holds it at 3.0 V either way.

This is written down because it is the kind of thing that is invisible while everything
works and is the first suspect when something drifts. It is not a reason to stop, and the
datasheet is explicit that an absolute maximum is a stress rating rather than a functional
one, with reliability affected only by extended exposure.

**This is also why jumper B, and not lead 1, is where the risk sits.** Lead 1 on a pin
printed `5V` would be inside the module's own VCC range and the regulator would absorb it.
The same 5 V arriving on CS through jumper B would be 1.7 V over the die's absolute maximum
with nothing in between. The jumper that looks like the harmless one is the one that converts
a supply mistake into a part mistake.

## What is not wired, and what that costs

The four-wire build needs a different set and is not described here, because it has not been
wired. Its chip select is the Arduino **D10** position, which this board maps to `&gpiod 14`,
corrected on Tuesday 6 October 2026 from a PA4 that is on no header at all and which the
build had accepted without complaint. A build accepting a pin says the processor can route
the signal there. It says nothing about whether a jumper lead can reach it.
