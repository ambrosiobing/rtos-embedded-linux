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

## Saturday 17 October 2026 at 12:10: the linker agrees with the chapter

The first build carrying the shield's power initialisation did not link:

```
app/libapp.a(shield_power.c.obj): in function `shield_power_init':
  undefined reference to `__device_dts_ord_42'
  undefined reference to `__device_dts_ord_43'
```

Those two ordinals are the pin controllers the supply-enable and low-power pins sit on, and they
were missing because **nothing in the build had asked for a pin controller**. The configuration
names a sensor and a log and nothing else, and the driver for this family pulls in no
general-purpose output of its own, so none was built.

**That is the chapter's claim restated by the linker.** The project had argued, from reading the
driver's source and its eleven-line binding, that the tree's driver for this family drives no
pin. A reader can doubt a reading of source. It is harder to doubt a build that contains that
driver and cannot resolve a pin controller, because there is none in it. The fix is one line of
configuration, and the line is worth its comment for that reason rather than for what it does.

**This was the cheapest of the four outcomes written down before the run**, which said that a
build failure means the description is wrong and is cheaper to find than either answer from the
hardware. It cost one build.

## Saturday 17 October 2026 at 12:17: the part answers, and the driver still cannot have it

The run that the morning was narrowing towards. Built 12:15:06, flashed, one press, and the two
boots the button's bounce produces are identical to the character.

```
<inf> shield_power: shield powered: supply enable on gpio@58020000 pin 7,
                    low power on gpio@58020400 pin 1, 10 ms
# bus scan on i2c@40005400, addresses 0x08 to 0x77
#   0x29 answers, which is the address the overlay names
#   1 device(s) answered
NOT READY vl53l5cx@29
<err> vl53l5cx: Failed to initialize the sensor (1)
```

**Four things are settled, and three of them were predictions.**

**The pins resolved to what the connector map said.** The supply enable came out on
`gpio@58020000` pin 7, which is PA7, and the low-power line on `gpio@58020400` pin 1, which is
PB1. Those are the Arduino D11 and A3 positions the vendor's library names, translated by this
board's own connector description rather than by anyone's memory, and the run prints them so the
translation is checkable rather than trusted.

**The address was 0x29 and the derivation was right.** The overlay said so, said it was derived
from the vendor's 0x52 by the seven-bit convention, and named 0x52 as the one other thing to try.
It is not needed. **One device answered and exactly one**, so the scan is not void and there is
nothing else on that bus.

**The part is powered and talking.** It acknowledges its address, which it could not do fifteen
minutes ago. The whole third branch of the earlier diagnosis, that a part held in its low-power
state cannot reply and no driver in the tree can release it, is now demonstrated rather than
argued: the only change between the silent part and the answering one is fifteen lines that drive
two pins the in-tree binding has no property for.

**And the driver still cannot initialise it, with a different error.** 255 before, **1** now. A
changed error on a part that now answers is not a failure repeating itself; it is the driver
getting further and stopping somewhere else. **That is the second branch of the prediction
written before the run**: the two generations differ in something the driver can see, and the
power question is eliminated rather than suspected.

### So the cheap attempt is answered, and the chapter's shape is settled by evidence

The question was whether the operating system's driver for the previous generation can drive this
part. It cannot, and the reason is no longer a guess about addresses or wiring: the part is
powered, on the bus, at the right address, answering, and the driver that has its firmware image
for the older part cannot bring the newer one up.

**The module argument, withdrawn this morning, comes back for a precise reason.** It was
withdrawn because the tree already carries this family's vendor library and a parallel copy would
have been a second way to do one thing. The tree carries the **previous generation's** library,
and the run has now shown that library cannot serve this part. So this project does need the
vendor's interface for this generation, and the reason is a demonstrated incompatibility rather
than an instinct about vendoring. That is a better justification than the chapter started with,
and it was obtained by trying the cheap thing first.

**Status 1 is `VL53L5CX_STATUS_TIMEOUT_ERROR`**, read from the vendor library's own header in
the workspace rather than translated from memory. The part answers its address and then something
in initialisation does not complete in time.

### The obvious reading has a competitor, and this bench has a prior for it

The obvious reading is that the previous generation's firmware image does not boot on this part,
and that would settle the chapter. **A timeout is also what a transfer problem looks like.**
Initialisation uploads tens of kilobytes over the two-wire bus, which is by far the largest
transfer this bench has ever asked of one, and the overlay had the bus at 400 kHz.

**This bench has already seen 400 kHz fail where 100 kHz worked**, on Friday 9 October 2026, with
a different part on flying leads. That is a recorded observation rather than a theory, and it
makes the bus speed a candidate that has to be ruled out before the part is held responsible.
Publishing
"the generations differ" on the strength of one timeout, with a known-suspicious bus speed
underneath it, would be exactly the kind of conclusion this volume exists not to publish.

So the next run is a control and not a fix: **the bus drops to 100 kHz and nothing else
changes.** One variable moves, and the driver's chunk size, which is the other candidate, waits
its turn. If the timeout survives the control, the reading that the two generations differ rests
on a control rather than on an inference, which is the difference between a chapter and an
opinion.

## Saturday 17 October 2026 at 12:42: the control is run, and the bus speed is ruled out

The bus at 100 kHz, nothing else changed, one press. The part still answers at 0x29, exactly one
device still answers, and initialisation still fails with `VL53L5CX_STATUS_TIMEOUT_ERROR`.

**So the speed is ruled out.** Quartering the bus rate did not rescue the initialisation, which
is what the control existed to find out, and the reading that the two generations differ no
longer rests on a bus speed this bench has seen misbehave before.

### The time it took to fail is the more useful half

| Bus rate | Time to the timeout |
|---|---|
| 400 kHz | 5.924 s |
| 100 kHz | 11.335 s |

**The failure took 1.91 times as long on a bus four times slower.** That rules out two things at
once. It is not all transfer, or it would have taken four times as long. And it is not all fixed
waiting, or it would have taken the same time. **Something substantial is being carried over the
bus before the timeout**, which means the driver is uploading the firmware image and the part is
accepting the traffic, and then something that does not depend on the bus at all runs out of
patience.

Split between a fixed part and a part that scales with the rate, the two times give about **1.8
seconds of transfer at 400 kHz and about 4.1 seconds of waiting that the bus rate does not
touch**. At 400 kHz, 1.8 seconds is the right order for the tens of kilobytes of firmware image
this part is sent.

**That decomposition fits exactly and the exactness means nothing**, which is worth saying
plainly: two measurements and a two-parameter model will always fit, so this is a reading of the
data rather than a test of it. **A third bus rate would test it**, and would be one more build
and one more press if the question ever needs settling. It does not need settling to support the
conclusion, because the conclusion rests on the control and not on the model: the timeout
survived a four-fold change in bus rate.

### What is now established, and what the chapter does next

The part is powered, on the bus, at the address the overlay derived, answering, and accepting a
large upload. The operating system's driver for the previous generation still cannot bring it up,
and the bus speed is not why. **The remaining explanation is the one the chapter was written
about**: this is a different part, and the firmware image and interface it needs are this
generation's rather than the previous one's.

The bus goes back to 400 kHz, since nothing now argues for the slower setting and every upload
costs less at the faster one.

## Saturday 17 October 2026, next: what the part says it is, asked before anything else asks it

Written before the run, as this project writes everything before the run.

The vendor library's own identification check is four bus transactions: select register page
zero, read the byte at register zero as a device identifier, read the byte at register one as a
revision, and put the page selector back. The previous generation answers **0xF0 and 0x02**, and
anything else makes the library report the part as not alive. All of that is read from
`vl53l5cx_api.c` in the workspace rather than from any memory of a register map.

So the cheapest thing a wired board can tell us is which of two situations this is:

| What the part answers | What it means |
|---|---|
| 0xF0 and 0x02 | the two generations **cannot be told apart at the handshake** and diverge only once a firmware image is booted. The in-tree driver cannot protect itself by checking, because this part will always look like one of its own |
| anything else | the handshake is where a driver should refuse, the in-tree one did not, and the chapter gains a line about why |

**It is read before the sensor driver initialises**, at a priority between the shield's power and
the driver's own, and that ordering is asserted at compile time. By the time an application's
`main()` runs, the driver has already spoken to this part, failed part way through an
initialisation and left a register page of its own selected. A reading taken then would be of a
part in an unknown state, and nothing in the output would say so.

The check writes the page selector back to the value the vendor's own routine leaves, so the
driver that follows meets what it would otherwise have met and this reading changes nothing about
the run it sits inside.

## Saturday 17 October 2026 at 13:23: the part names itself, and it is neither of my two answers

```
<inf> identify: the part identifies as device 0xf0 revision 0x0c
<inf> identify: the previous generation answers device 0xf0 revision 0x02
```

**The device identifier is the same and the revision is not.** 0xF0 in both; 0x0C here against
the 0x02 the previous generation answers. The page before this one offered two outcomes, the same
two bytes or different ones, and the truth is a third thing that is more useful than either:
**the two parts agree on what family they are and disagree on which member.**

That is exactly the shape a generation change takes, and it is the first hard evidence in this
project that the part on the bench is a different member of one family rather than a different
family or a misbehaving example of the same one. Everything before it was inference from a
failure; this is the part's own answer, read in four bus transactions before anything else spoke
to it.

### The vendor's own check would have refused this part in four transactions

`vl53l5cx_is_alive` requires **both** bytes to match and reports the part as not alive otherwise.
So the library the in-tree driver links against already contains a cheap and correct refusal for
exactly this case.

**And the driver uploaded tens of kilobytes anyway, then timed out after 5.924 seconds.** Twice,
to the millisecond, across two boots. That is the observation this run adds beyond the revision
byte: a part that could have been refused in four transactions and one millisecond was instead
sent a firmware image it cannot boot, and the failure arrived six seconds later as a timeout,
which names neither the part nor the reason.

**It never calls the check.** The driver's own initialisation is thirteen lines: it asks whether
the bus is ready, calls the vendor's `vl53l5cx_init` and reports the status it gets back. A
search of that file for `is_alive` returns nothing. So this is not a deliberate choice to
continue past a failed identification; the identification is simply not asked for.

That makes it a specific and checkable observation about upstream code rather than an opinion:
**a four-transaction refusal exists in the library the driver links against, and the driver goes
straight to uploading a firmware image instead.** On a part of the right family and the wrong
revision, which is what sits on this bench, the difference is a clear refusal in about a
millisecond against a timeout in 5.924 seconds that names neither the part nor the reason.

**Whether to offer that upstream is a decision for Joseph and not for this page.** It is written
down here because it was found, with the evidence beside it, and because the chapter's own
binding should carry the refusal the tree's does not.

### What this does to the chapter

The chapter's claim was that the tree knows the previous generation and not this one. It now has
the part's own word for the difference, a number for it, and a specific place where a driver
could tell the two apart before doing anything expensive. **The binding this project writes
should carry that refusal**, because a driver that uploads an image to a part it could have
identified as wrong is a driver that fails slowly and says little, and six seconds of silence
followed by "timeout" is what that looks like from a console.

## Saturday 17 October 2026, the binding, written and not yet used

[dts/bindings/sensor/st,vl53l8cx.yaml](../dts/bindings/sensor/st,vl53l8cx.yaml) exists and
nothing points at it yet, which is deliberate: a node carrying a compatible with no driver behind
it produces no device, and the link would fail on the application's own alias. It is committed
now because it is the artefact the chapter is about and it can be read and argued with before a
line of driver exists.

**Three properties, and they are the whole difference from the binding the tree carries for the
previous generation.** That one is eleven lines and has no property of its own, so it describes a
part that is always powered, never in a low-power state, and never asks to be read. On this
expansion board all three are false, and the 12:17 run is the evidence rather than the argument:
with nothing driving the supply enable the part did not acknowledge its address, and with two
pins driven it answered at once.

All three are optional, because a binding describes what a board may have rather than what one
board does have, and each says what a driver does when it is absent. The data-ready line is the
one the fourth acceptance criterion turns on, and its description says so.

The build now states its binding directory rather than relying on the operating system finding
it, which is P04's lesson moved across: that project added an overlay to a build directory
configured days earlier, the file was never found, and the build silently differed from the tree
by which files happened to exist at first configure.

## Saturday 17 October 2026, this project's own driver, written and not yet run

[src/vl53l8cx.c](../src/vl53l8cx.c) binds to the new compatible, drives the supply enable and the
low-power line from its own binding, waits the vendor's ten milliseconds, asks the part what it
is, and either refuses it by name or binds. **It does not range.** Ranging needs this
generation's vendor interface, which is not in the tree and has to be cloned at a pin; nothing in
this file depends on that and all of it is runnable now.

**The refusal is the point of the file.** Three cases, and they are chosen by what can be
justified rather than by what would be tidy:

| What the part answers | What the driver does |
|---|---|
| a family byte that is not this family's | refuses. Whatever is at that address, no firmware image should be sent to it |
| the previous generation's revision | refuses **by name**, and the message says to describe it as the compatible the tree already carries rather than that something went wrong |
| anything else | binds, and logs the revision |

**The third case is deliberate and the reason is a sample of one.** This bench has met one part
of this generation and it answers revision 0x0C. One example is not a specification, so the
driver does not refuse an unknown revision: it would be refusing on the strength of n = 1, and a
reader with a later part would be told their hardware was wrong by a file that had never seen it.
It logs what it found and says it has met one, which is the honest form of the same information.

**`sample_fetch` returns "not supported" rather than zeroes**, which is this project's first
acceptance criterion in one line: a grid of zeroes looks like an empty room and is more
convincing than a single zero for having structure.

### One application, two descriptions

[overlays/own_driver.overlay](../overlays/own_driver.overlay) joins the first one, and the two
differ in exactly two things. The compatible string, which is the claim that these are different
parts. And where the control pins live: in the node here, where this project's binding declares
them, and in `zephyr,user` there, because the tree's binding for the previous generation has no
property for them.

**Nothing in [src/main.c](../src/main.c) changes between them**, which is P03's claim arriving
here rather than being restated: the application knows one alias and the description decides
everything else. The two early-initialisation helpers compile to nothing under the second
description, because the driver drives those pins itself and two places driving one pin is how
descriptions and code drift apart.

**The data-ready line is absent from both overlays and that is a statement.** This project has
not established which connector position carries it on this expansion board, and the binding says
a driver polls when it is absent. The fourth acceptance criterion needs it and it will be added
with its source, as every other pin in this volume has been.

## Saturday 17 October 2026, a configure that failed for a reason outside the application

The first build of the new description stopped before it compiled anything:

```
Loading Zephyr default modules (Freestanding).
-- Found Python3: /usr/bin/python3
boards/st/nucleo_h7a3zi_q/nucleo_h7a3zi_q.dts:9:10: fatal error:
  st/h7/stm32h7a3z(g-i)txq-pinctrl.dtsi: No such file or directory
```

**The missing file belongs to the vendor's hardware abstraction module and the board's own
description includes it**, so this is not a fault in the driver, the binding or the overlay: the
build never reached them. The tell is two lines above the error. Python came from `/usr/bin`
rather than from the workspace, and there is no "Found west" line at all, where every successful
configure in this project has one.

**West lives in the workspace's virtual environment**, which this shell had not activated. With
no west, the build cannot ask the workspace what modules exist, loads none, and the vendor's
devicetree directory is not on the include path. The same absence stopped `west blobs fetch`
earlier the same day, and it stayed invisible in between because **an existing build directory
keeps working**: its module paths were resolved at its first configure and cached, so
`ninja -C build-p05-existing` ran happily all afternoon while a fresh configure could not.

That is worth the paragraph because of the shape rather than the fix. A toolchain fault that
only appears on a new build directory looks exactly like a fault in whatever was new in that
directory, which here would have been a driver and a binding written an hour earlier.
