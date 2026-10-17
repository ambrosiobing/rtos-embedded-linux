# P05 bench log

This is the narrative record: what was run, what came back, and what each result changed. The
figures, when there are any, go in RESULTS.md. Nothing has run on hardware yet.

## Saturday 17 October 2026: the firmware image, and the cheap attempt links

Two results, and the second is stronger than it looks.

**The firmware image is in the workspace.** `west blobs fetch hal_st` took one file:

```
Fetching blob hal_st:
  ~/zephyrproject/modules/hal/st/zephyr/blobs/sensor/vl53l5cx/vl53l5cx_buffers.h
hal_st A sensor/vl53l5cx/vl53l5cx_buffers.h lib
```

Two things were shortened to fit the page and both are said rather than hidden: the home
directory is written `~`, and the second line ended with the same path again, which is left off.

That is the buffer header the vendor library ships without, carrying the binary image this part
uploads at initialisation. The operating system's rules do not admit a blob in its source, so it
is fetched into the workspace and is not in this repository either, which this volume's own rule
about vendoring would have required anyway. **West had to be found first**: it lives in the
workspace's virtual environment, and the earlier attempt failed with "command not found" in a
shell that had not activated it. Every Zephyr build so far had gone through plain `cmake`, so the
absence stayed invisible until something needed west itself.

**The cheap attempt links.** The part on this bench is described with the compatible string of
the generation before it, and the image builds:

```
-- Found devicetree overlay: overlays/existing_driver.overlay
[184/184] Linking C executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:      138964 B         2 MB      6.63%
             RAM:       13888 B       256 KB      5.30%
```

**The link succeeding is the proof that the in-tree driver was compiled in, and that is worth
stating rather than assuming.** The application takes its device from a devicetree alias through
`DEVICE_DT_GET`. If the driver had not been enabled, that macro would still have compiled and the
link would have failed on a device that no driver defines. So the chain holds end to end: the
overlay describes a part, the description enables the driver, the driver finds the vendor library
in the module, and the vendor library finds the image that was just fetched. None of that is a
statement about the hardware, which has not been powered yet.

**The image is 138964 bytes where P04's was 31412**, on the same board with a comparable
configuration. Most of the difference is this part's firmware image and the vendor library around
it, which is a product fact rather than a curiosity: a part whose driver is open and whose
firmware is not also costs flash for an image the host has to carry and upload. The exact split
is read off a size report rather than estimated here.

One warning is deliberate: `__ASSERT() statements are globally ENABLED`, from `CONFIG_ASSERT=y`.
P04 turned assertions off because they cost counts inside a measured interval. Nothing here is
measured.

### What the next run can return, written before it runs

The shield is not fitted yet. When it is, the identification run has four outcomes and they are
distinguishable, which is the reason for writing them down first:

| Outcome | What it means | What to change |
|---|---|---|
| READY | the existing driver binds and initialises this part, and the question becomes how far that goes | nothing; read the grid next |
| NOT READY, log names the image | the image did not reach the part | already fetched, so this would be a build that did not pick it up |
| NOT READY, no answer on the bus | the address is wrong, the interface jumper is on the other bus, or the part is held in its low-power state | 0x52 in the overlay, then the jumper, then the control line |
| build failure | the description is wrong | the overlay |

**The third row has three causes and they are ordered by cost rather than likelihood.** The
address is one digit in a file. The jumper is one look at the board. The control line is the
binding this project exists to write, and if that is the answer then the run has earned the
chapter.

## Saturday 17 October 2026 at 11:16: the first run, which is not interpretable yet

Built 11:09:43, flashed, one press. The console:

```
p05 the zones, identification run
# built Oct 17 2026 11:09:43, and a stamp older than your last edit means a stale flash
NOT READY vl53l5cx@29
[00:00:02.131,000] <err> vl53l5cx: Failed to initialize the sensor (255)
```

**The bench state was reported in two steps and the first reading of it here was wrong, so it is
corrected rather than quietly replaced.** This page first said that both of P04's leads were
still fitted and that one of them, in CN7 pin 19, would stop a shield seating. **Only the ground
lead is connected**, from a ground pin on the NUCLEO to the ground terminal of the acquisition
board's CH0 block; the signal lead that was on CN7 pin 19 is off.

**That changes the reading, and in the part's favour.** A ground lead between two boards cannot
stop a two-wire bus working. The parts of this project that depend on the bus are therefore
testable with it in place, and leaving it is better than removing it, since it is the lead that
has to be there whenever the acquisition board witnesses anything. The one thing worth checking
is which ground socket it occupies: a shield pin cannot enter a socket a lead is already in, so a
ground pin on a header the shield seats into should move to one the shield does not cover, and a
shield has several ground pins so a single missed one changes nothing electrically.

**What is still unconfirmed is whether the shield is seated at all**, which is the fact this run
turns on, and the rule `bench state is not the record` is why it is not assumed in either
direction here. If the shield is on the board and flat, this run stands as a real result. If it
is not, the run says nothing about the part.

So the reading, pending that one fact: What the run does establish is about the
software and not the hardware: the image boots, the application runs, the driver binds, the
driver's own initialisation is reached and fails, and **the failure code is the vendor library's
generic error, 255**, which distinguishes none of the three causes written down in advance.

### The generic error is the finding, and it changed the application

A part at a different address, a part held in its low-power state, and a part whose interface
jumper selects the four-wire bus **all fail identically** from inside a driver that only ever
talks to one address. The project had written those three causes down before the run and the run
could not tell them apart, which is a defect in the experiment rather than in the part.

**A bus scan separates them in one run**, so the application now scans before it asks, and the
scan is built to report its own failure: a probe is a zero-length write, a controller that does
not implement one can answer for every address, and a scan that could not notice that would hand
back a list of 112 devices and look authoritative. Four outcomes, each pointing somewhere
different:

| The scan says | What it means |
|---|---|
| nothing answered | the part is not on this bus. Power, the low-power line, or the jumper. No address is wrong, because none is right |
| the address the overlay names | the overlay is right and initialisation failed, which points at the firmware image or at a real difference between the generations |
| some other address | the overlay is wrong and the scan has printed the right answer |
| almost everything | the scan is void on this controller, and it says so rather than reporting sensors |

One smaller fix came with it. **The driver's error landed below this application's advice**,
because initialisation happens at boot and the log is flushed asynchronously, so the console read
as advice first and diagnosis second. The application now waits 200 milliseconds before it prints
anything, which puts the console back in the order things happened.

## Saturday 17 October 2026, from the vendor's own sources: the run is explained

Three documents answered what the bench could not, and together they close the question the
11:16 run opened. None of them is a datasheet summary: the quick start guide is ST's own, and the
library is ST's own Arduino support for this exact shield.

**The part is held unpowered until a pin is driven, and the tree's driver drives no pin.** ST's
Arduino library for this shield defines `PWREN_PIN 11` and `LPN_PIN A3`, and its simplest example
drives the supply enable high and waits ten milliseconds **before** the sensor object is
constructed:

```
#define LPN_PIN A3
#define PWREN_PIN 11
...
pinMode(PWREN_PIN, OUTPUT);
digitalWrite(PWREN_PIN, HIGH);
delay(10);
```

**So the cheap attempt is answered, and the answer is no.** The in-tree driver for the previous
generation cannot bring this part up on this shield at any address, because the part is not
powered when the driver first speaks to it and nothing in that driver can power it. The project
wrote three candidate causes down before the run and this is the third of them, which is the one
that was described in advance as "the clearest possible argument for the binding this project
writes". It has turned out to be exactly that.

**The address was never the question.** It may still be wrong and the scan will say so, but a
scan taken with the supply enable low will find nothing whatever the address is, which is why the
scan now has a row for "nothing answered" that points at power rather than at addressing.

**The interface is three jumpers and no soldering iron.** ST's library README: to use the shield
in the four-wire mode, J7, J8 and J9 must be moved to SPI. So the two-wire mode is the delivered
position and the chapter's stated gap, that the bus selection might need an iron this bench does
not have, does not exist. That sentence in the chapter is wrong and will be corrected with this
as the source.

### The cover glass is an accessory, and it stays off

ST's quick start guide lists among the parts on the board: **0.25, 0.5 and 1 mm spacers to
simulate air gaps, with the cover glasses**, and its photograph labels the spacers and the cover
glass as separate items beside the board rather than as something fitted over the sensor. So it
is not a protective film to be peeled off before use. **It is a deliberate optical configuration
for asking what happens when the part sits behind a product's window**, and the air gap is varied
with the spacers.

It stays off for every run in this project until a run is specifically about it, and the record
says so, because an optical path is part of a measurement setup exactly as a wire is. Two things
make that the right default rather than the lazy one. The vendor's own evaluation software
carries a `CoverGlass On` setting, which is unticked in the guide's own screenshot, so a cover
glass is a mode rather than a neutral addition. And the vendor library in the workspace carries a
crosstalk calibration plugin, which is there because a window reflects some of the light straight
back into the receiver and that has to be calibrated out before any figure means anything.

**So "with a cover glass, calibrated" is a variant worth building and a poor default**, and it is
the kind of variant this volume writes down rather than performs by accident.

### One more fact that renames the part

The guide's key products line reads that the board carries a **noncommercial VL53L8CA
super-sensor used for evaluation only**, and that the board supports the CX and CH behaviours. So
the part on this bench is an evaluation superset rather than the catalogue part, and the chapter
should name it that way. It does not change the premise, since the tree has no driver for any
member of this generation, and it does change what a reader should expect to reproduce with a
different shield.
