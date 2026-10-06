# Wiring the SEN0032 to the NUCLEO-H7A3ZI-Q

Written on Tuesday 6 October 2026, from the vendor's own schematic and the Analog Devices
datasheet rather than from another maker's ADXL345 breakout, because the pad order and the
supply arrangement both vary between makers and getting either wrong is expensive.

Nothing is soldered. The board is unplugged from USB while leads are moved.

## The two-wire wiring, for the part at 0x53

Six leads. The two that are easy to leave off are the last two, and leaving either off costs
an evening rather than a part.

| Lead | Colour | SEN0032 pad | Nucleo pin | Signal and direction |
|---|---|---|---|---|
| 1 | red | 2, **VCC** | **3V3** | supply, board to module |
| 2 | black | 1, **GND** | **GND** | common return |
| 3 | yellow | 8, **SCL** | **PB8**, the Arduino **D15** position | clock, board to module |
| 4 | green | 7, **SDA** | **PB9**, the Arduino **D14** position | data, both directions |
| 5 | black | 6, **SDO** | **GND** | address select held low, which makes the part 0x53 |
| 6 | red | 3, **CS** | **3V3** | bus select held high, which puts the part in two-wire mode |

Pads 4 and 5, **INT1** and **INT2**, stay unconnected. The trigger build of criterion 5 will
want one of them; nothing before that does.

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

## What is not wired, and what that costs

The four-wire build needs a different set and is not described here, because it has not been
wired. Its chip select is the Arduino **D10** position, which this board maps to `&gpiod 14`,
corrected on Tuesday 6 October 2026 from a PA4 that is on no header at all and which the
build had accepted without complaint. A build accepting a pin says the processor can route
the signal there. It says nothing about whether a jumper lead can reach it.
