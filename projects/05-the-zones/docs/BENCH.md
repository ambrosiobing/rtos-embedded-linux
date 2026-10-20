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

## Saturday 17 October 2026, a configure that failed, and two wrong readings before the right one

The first build of the new description stopped before it compiled anything:

```
boards/st/nucleo_h7a3zi_q/nucleo_h7a3zi_q.dts:9:10: fatal error:
  st/h7/stm32h7a3z(g-i)txq-pinctrl.dtsi: No such file or directory
```

That file belongs to the vendor's hardware abstraction module and the board's own description
includes it, so the build never reached this project's driver, binding or overlay. **It took four
configures to say why, and two readings were written up before they had been tested.**

| | Workspace environment | Build directory | Result |
|---|---|---|---|
| 1 | not activated | fresh | fails |
| 2 | activated | **reused from 1** | fails |
| 3 | not activated | fresh | fails |
| 4 | activated | fresh | **configures** |

**The environment is the cause.** West lives in the workspace's virtual environment on this
machine; without it the build cannot ask what modules exist, so the vendor's devicetree directory
is not on the include path and the board's own description cannot be preprocessed. The file is on
disk the whole time, under that vendor module, which the fourth run also confirmed by finding it.

### The two wrong readings, kept because they are the lesson

**The first was right and was published before it was tested.** It came from two lines near the
top of the output: a Python from the system rather than the workspace, and no line reporting
west. A story fitted them and the story went into this page, the build file and a memory with no
experiment behind it. It happened to be correct, which is the least useful way to be right.

**The second was the retraction, and it was wrong because its control was contaminated.** Row 2
looks like a test of the environment and is not one: it reused the build directory from row 1, so
cmake read a cached configure and re-examined nothing. The output says as much, in the absence of
the very lines whose absence had been the evidence. **A control that reuses the state under test
is not a control**, and on the strength of it a correct claim was withdrawn, two files were
edited and a memory was rewritten.

Both errors have one root and it is not the guessing. It is publishing a reading before a clean
experiment, which is what this page spent the afternoon refusing to do with the sensor and then
did twice in twenty minutes with the toolchain.

**One thing is not isolated and is written as not isolated.** The successful configure differs
from the first failure in two ways: the environment, and the removal of a line this project had
added to its build file. Row 3 shows that removing the line alone does not help, so the
environment is necessary. Whether the line would have been harmless with the environment present
was never tested, because the only run that could have said reused a cache. The line was
redundant in any case and is gone, so that question is closed by not mattering rather than by
being answered.

## Saturday 17 October 2026, the first compile of this project's own driver

It configured, compiled and did not link:

```
app/libapp.a(main.c.obj): undefined reference to `__device_dts_ord_31'
app/libapp.a(vl53l8cx.c.obj):(.rodata.vl53l8cx_config_0+0x0):
  undefined reference to `__device_dts_ord_31'
```

**That ordinal is the two-wire controller the part sits on**, and it is absent from the image for
the same reason the pin controllers were absent two hours earlier: nothing asked for it. The link
line shows the gap plainly, carrying the pin and serial and timer drivers and no bus driver at
all.

**The build that uses the tree's driver never needed the line, and that difference is the
finding.** An in-tree driver has a Kconfig of its own and that one reads `select I2C`, so the
controller arrives with the driver. **A source file in an application has no Kconfig and can
select nothing**, so everything it depends on has to be named in the application's configuration.

That is a concrete argument for this driver eventually being a module with its own Kconfig rather
than a file in an application, and it is a better argument than tidiness: a driver should carry
its own dependencies, and a reader who takes the driver without taking the configuration beside
it should not be met with a link error naming a devicetree ordinal.

**It is also the second time today that a missing `select` has been the first thing a build
said.** The two together are the shape of writing a driver outside the tree, and the chapter
should say so once rather than treat each as an accident.

## Saturday 17 October 2026 at 14:28: this project's own driver binds the part

Built 14:28:17, flashed, one press:

```
# bus scan on i2c@40005400, addresses 0x08 to 0x77
#   0x29 answers, which is the address the overlay names
#   1 device(s) answered
READY vl53l8cx@29
<inf> vl53l8cx: device 0xf0 revision 0x0c at 0x29
<inf> vl53l8cx: identified and bound; this driver does not range yet
```

**Where the tree's driver reported a timeout, this one reports the part.** Same board, same
shield, same address, same part. The two descriptions differ in a compatible string and in where
two control pins are declared, and the console goes from six seconds of silence followed by a
bare `(1)` to a device identified by name in ten milliseconds.

| | The tree's driver | This project's |
|---|---|---|
| the control pins | not in its binding, so nothing drives them | in the binding, driven by the driver |
| the identity | never read | read in four transactions |
| a part it cannot drive | a firmware image, then a timeout naming nothing | refused by name, or bound and logged |
| what the console says | `Failed to initialize the sensor (1)` | `device 0xf0 revision 0x0c at 0x29` |

**The image is 47424 bytes against the other's 144624, and that is not a claim about either
driver.** The difference is the vendor library and the firmware image, which this one does not
carry **because it does not range yet**. When it does, it will carry them and the figure will
move. Quoting the gap as a saving would be quoting an unfinished feature.

### Two defects of mine, both found by this run and neither by the firmware

**The application's success message was true of one description and not the other.** It read that
the driver the tree already carries binds and initialises this part, which is false of the
description that printed it. One application serving two descriptions may not carry a sentence
that is only true of one of them. It now says that a driver bound the node and that the node's
name says which, since the console already prints `vl53l5cx@29` or `vl53l8cx@29`.

That is the failure this whole volume watches for: prose that was true when written and is not
true of what the code now does, with nothing able to notice.

**Reflashed at 14:43 to confirm the correction**, because this page quotes that console and
quoting a line the code no longer prints is precisely the drift being described. Three boots,
identical, with the corrected sentence and the two device lines unchanged. The image grew by 68
bytes, which is the new text. A correction to a console message is worth a reflash for the same
reason a build stamp is worth a line: the record has to be of what ran.

**And the edit that fixed it broke five string literals**, because it was made through a shell
heredoc, which ate a backslash level and turned every escape for a newline into an actual
newline. That trap is on file from earlier work and was walked into anyway.
`check_string_literals.py` reported ten problems immediately, and the repair was made with a
proper editor rather than another heredoc, which is what the note on file says to do.

**Then this paragraph was written through a heredoc and broke in exactly the same way**, which is
recorded rather than silently repaired: the sentence describing the trap contained it. Hence no
escape character appears in the sentence above, and the fix for both was the same editor.

## Saturday 17 October 2026, the vendor's interface for this generation, written and not yet built

Ranging needs the library this generation's part was made for, and the tree carries only the
previous one's. **Nothing is vendored**, which is this volume's rule and the operating system's
as well: the library is cloned at a pin and compiled from where it lies, and neither it nor the
firmware image it carries enters this repository. P04 brings in a kernel and a board support
package the same way.

A missing clone is reported by the build with the command that fixes it, rather than arriving as
a compiler that cannot find a header, which names a symptom and not a step.

**The porting layer is six functions and a structure**, and the structure is this project's
rather than the vendor's reference one. ST's own carries three function pointers because their
board support reaches hardware through a layer of its own; nothing here needs that, since a
devicetree address specification already names the controller and the address together. The
operating system's driver for the previous generation makes the same choice.

Two of the six are not three lines, and the reason is the two large transfers: initialisation
sends tens of kilobytes to the part and a result comes back in one piece, so both are chunked
with the register address advanced by what has already gone. **A write does not copy**: the
address and the payload travel as two messages of one transaction rather than being joined into
a buffer, because joining them would mean owning a second copy of the firmware image.

**What this project does not know yet, and will be told by a compiler.** The vendor's interface
may want more from a porting header than those six: a count of targets per zone, a buffer size,
a compile-time option. None of it is guessed. The first build will name each thing it cannot
find, which is a better source than a reading of somebody else's header, and each will be added
with the line of theirs that asked for it.

### The order in the driver is load bearing

The identification stays in front of the vendor's initialisation and that is the argument rather
than a style. Four bus transactions and a millisecond decide whether this part is one this driver
knows; only then does it send the image, which is tens of kilobytes and seconds. **The driver in
the tree does the expensive thing first and the cheap thing not at all**, which is why a wrong
part there costs six seconds and a bare number.

## Saturday 17 October 2026, eight red pushes nobody looked at

Continuous integration had failed on every push since 13:40, and the author found out from a
mailbox at 14:57. **One file and one reason**: the published tree's allowlist of file types
admits `.yml` and this project's devicetree binding is a `.yaml`, which is the spelling the
operating system requires for a binding. The first failure is the commit that added it.

The allowlist now admits `.yaml`, and that is a clarification rather than a relaxation. It
already admits `.conf` and `.overlay`, the other two files a project here uses to describe
hardware to the operating system, and a binding is the third of that set. The purpose of the
guard is untouched: no PDF, no HTML, no LaTeX.

### The file type was the symptom and the invisibility was the fault

Everything else that workflow runs is a script the authoring laptop can run before a commit, and
it had been run before every one of those eight pushes. **The two rules that failed existed only
as shell inside the workflow**, so they could not be run anywhere except by pushing, which means
they are discovered by email or not at all.

They are now `scripts/check_tree.py`, with their original comments carried over because the
reasoning is the part worth keeping, and the workflow calls it. Nothing about either rule changed
in the move except the `.yaml`.

**The habit this leaves** is to run every check the workflow runs, not the subset that happens to
be a script, and the way to make that possible is to keep no rule anywhere a laptop cannot reach.
A guard the author cannot run is a guard the author will not run.

## Saturday 17 October 2026 at 15:20: the part accepts this generation's firmware image

```
<inf> vl53l8cx: device 0xf0 revision 0x0c at 0x29
READY vl53l8cx@29
[00:00:01.970,000] <inf> vl53l8cx: initialised with this generation's own interface
```

**The claim is now proven in both directions on one bench, one board and one part.** The
previous generation's library sends its firmware image to this part and times out after 5.924
seconds. This generation's library sends its own and the part is initialised in 1.971 seconds.
Everything else is held fixed: the same board, the same shield, the same bus at the same rate,
the same address, the same two control pins driven the same way by the same code, and a porting
layer that differs only in the name of the library it serves.

| | The previous generation's library | This generation's |
|---|---|---|
| what it does with the part | uploads an image the part cannot boot | uploads one it can |
| how it ends | `VL53L5CX_STATUS_TIMEOUT_ERROR` | initialised |
| how long it takes | 5.924 s | 1.971 s |

**That difference is the whole project in two numbers**, and the chapter can now say the thing it
set out to say without any hedging: the operating system knows the generation before this one,
this part is not that part, and the gap is a driver and a binding and the right vendor interface.

### It also corroborates a model that was explicitly not tested

The timing decomposition written at 12:42 split the failure into about **1.8 seconds of transfer
at 400 kHz and about 4.1 seconds of waiting that the bus rate does not touch**, and that page
said plainly that two measurements and a two-parameter model always fit, so it was a reading of
the data and not a test of it.

**A successful initialisation takes 1.971 seconds.** That is the same work without the waiting:
the image goes over the same bus at the same rate, and the part answers instead of being waited
for. It lands within a tenth of a second of the transfer term the model predicted.

That is corroboration and not proof, and the difference matters enough to write down. The model
was fitted to two points of one experiment and this is a third point from a different one, which
is a much better position than the model had an hour ago, but a third bus rate would still be
the test. What it does settle is the shape: **the previous generation's library was not failing
to send the image. It sent it, and then waited for a part that was never going to answer.**

### What is now true of the four candidate causes written before any of this

Every one has been closed by a run rather than by argument: the address was right, the part is
powered by two pins the tree's binding has no property for, the bus speed is not the cause, and
the part is a different member of the same family that accepts its own generation's image. The
only thing this driver has not done is read the grid.

## Saturday 17 October 2026 at 15:35: a grid, and the two boots disagree usefully

The first data this project has produced. One button press gives two boots, and they are not the
same thing at all.

**The second boot is a measurement.** Sixty-four distances between 2540 and 2640 millimetres,
every zone reporting one target, every status 6. It is coherent in a way noise is not: the eight
row means run 2596, 2582, 2577, 2576, 2577, 2570, 2569, 2578, which is flat, while the eight
column means run 2557, 2557, 2565, 2570, 2582, 2584, 2602, 2609, which rises by 52 millimetres
from one side to the other. **That is what a flat surface at about two and a half metres looks
like when the sensor is turned a few degrees about its vertical axis**, and nothing else in this
project could have produced it.

**The first boot is not a measurement.** The same sixty-four zones read between -2 and +5
millimetres, with **ten of them negative**, every zone still claiming one target and every status
still 6.

| | First boot | Second boot |
|---|---|---|
| range of the grid | -2 to 5 mm | 2540 to 2640 mm |
| zones reading negative | 10 of 64 | none |
| targets per zone | 1 everywhere | 1 everywhere |
| status | 6 everywhere | 6 everywhere |

### Two things follow and the second is uncomfortable

**A processor reset does not reset this part.** The two boots come from one press, the second
within seconds of the first, and the part keeps ranging across a reset because nothing in the
reset reaches its supply. So the first boot asked a part that had just been told to start, and
the second asked one that had been ranging for seconds. That is the candidate mechanism, it is
not proven, and the test is a full power cycle with the cable out rather than a button press.
This bench has met the shape before: the programmer's buffer survives a flash, and a stale image
reads as a reproduction.

**The status did not distinguish them, and the second acceptance criterion is about exactly
that.** Six on a coherent surface at two and a half metres, and six on ten negative millimetres.
Whatever 6 means, it does not mean "this number is a measurement", and a consumer that filtered
on status alone would have taken the first grid as real. **No mapping is written here**, because
what the values mean comes from the vendor's documentation and this is an observation about one
value in one run, not a reading of their meaning.

### What changed in the application because of it

It now takes **four frames in one run** rather than one, polls for each with a stated bound, and
prints how long each took and when it arrived. One run then shows the settling instead of leaving
a reader to infer it from two logs, and the interval between frames is the ranging period
measured rather than assumed, which is the figure the driver deliberately did not set.

## Saturday 17 October 2026 at 15:44: a cold start, and the part is not there at all

The control asked for in the previous entry: cable out, a few seconds, cable back in, no button
pressed. **Nothing answered anywhere on the bus.**

```
# bus scan on i2c@40005400, addresses 0x08 to 0x77
#   nothing answered. The part is not on this bus
#   0 device(s) answered
NOT READY vl53l8cx@29
<err> vl53l8cx: no answer at 0x29: the part is not powered, is held in its
      low-power state, or is not at this address
```

**That is not the result the control was looking for and it is a better one.** The question was
whether a cold part's first frame is noise. The answer is that a cold part does not reach the
first frame: it never answers its address.

### Every run before this one was a processor reset on a board that never lost power

The board has been powered continuously since the shield was fitted. Every result in this log
came from a button press, which resets the processor and leaves the part's supply alone, so the
part had been powered for minutes or hours before anything spoke to it. **The driver's ten
millisecond settle was never tested from cold until now**, and from cold it is not enough.

The vendor's own example waits ten milliseconds after driving the supply enable, which this
driver copied deliberately rather than inventing a figure. On an Arduino that ten milliseconds
follows a rail stable since whenever the sketch reached its setup. On a board coming up from cold
it follows a rail still settling, and the two are not the same ten milliseconds.

**So a copied figure was right about the number and wrong about the condition**, which is a
specific kind of mistake worth naming: the value came from a source, and the circumstances it was
measured in did not.

### The driver now asks repeatedly and reports the figure

A longer fixed delay would be a number chosen to be safe, and nobody would ever learn what the
part actually needs. The driver polls for up to half a second and **prints how long it waited
whenever that is not zero**, so the next cold start produces the figure rather than hiding it
behind a delay. If half a second is not enough, that is also a result and the bound is stated in
the message.

**What this does not yet explain.** Whether the part needs the time because its supply is
settling, because it has its own boot, or because the supply enable needs holding longer before
it means anything. The figure will narrow that, and no mechanism is offered until it does.

## Saturday 17 October 2026 at 15:50: half a second is not enough either, and a confound surfaces

```
<err> vl53l8cx: no answer at 0x29 after 505 ms
```

**Five hundred milliseconds is far too long for a settling rail**, so the explanation offered in
the previous entry is not the explanation. Something else is keeping this part off the bus from
cold.

### A conclusion published earlier had two changes in it and was credited to one

At 11:16 the part did not answer. At 12:17 it did, and this log says the difference was fifteen
lines driving a supply enable and a low-power line. **Two things changed between those runs**: the
code, and the confirmation that the shield was properly seated, which was settled in between.

The reading was written as though only the code had changed. It may still be right, and it is no
longer supported the way it was written, which is the part that matters. **This is the project's
own rule applied to the project's own conclusion**: a control that moves two things tells you
about neither.

The cold-start failure is what surfaced it, which is the usual way: a later run disagreeing with
an earlier reading is the cheapest audit there is.

### The next run separates slow from wrong

The bound is raised from half a second to five, and that is an experiment and not a fix. Either
the part answers at some point and the figure is the finding, or it does not answer in five
seconds and the bound is not the thing to change: the pins, their order, or something about the
shield that this project has not read would be.

**What is deliberately not done yet.** No pin is inverted, no order is swapped and no second
delay is inserted, because each would be a guess and two guesses at once would be worse. The
run decides which question to ask.

## Saturday 17 October 2026 at 15:56: five seconds and nothing, so the part is not slow

```
<err> vl53l8cx: no answer at 0x29 after 5003 ms
```

Ten milliseconds, five hundred, five thousand: from a cold start this part never answers. **The
bound was not the thing to change**, which the previous entry said would be the reading if this
came back at five seconds, so that is what it is read as.

### A defect in the build stamp, found because the stamp disagreed with the code

The console of that run says it was built at 15:42:20 and the message it printed, with its five
thousand millisecond bound, exists only in code written at 15:55. Both are true: **the stamp is
`__DATE__` and `__TIME__` compiled into `main.c`**, so it records when that one file was last
compiled, and the change was in the driver. A run whose application is unchanged carries an old
stamp in a new image.

That is worth fixing and is not fixed here. The stamp was introduced in P04 to tell a stale flash
from a reproduction, and it does that job only for changes that touch the file it lives in. **A
stamp that can be older than the image it is in is a weaker guard than it looks**, and the honest
version comes from the build rather than from a translation unit.

### The control that is one build away

`overlays/own_driver_nopins.overlay` is `own_driver.overlay` with the two control-pin properties
removed and nothing else changed. The binding declares both optional and says a driver that finds
them absent drives nothing, so that description produces a driver which touches no pin.

**The thing most in doubt is this project's own pin driving.** The conclusion that two pins woke
the part came from a pair of runs between which two things changed, the pins and the
confirmation that the shield was seated, and it was credited to the pins.

| The control says | What follows |
|---|---|
| the part answers with no pin driven | driving those pins is what stops it: a level, an order or a meaning is wrong, and the earlier conclusion was an artefact of a control that moved two things |
| it stays silent | the pins are not the difference either way, and the cold start fails for a reason not yet looked at: the interface jumpers, a current-measurement link on the shield, or its own power path |

## Saturday 17 October 2026 at 16:08: no pins driven, and the part is still not there

The control ran and its answer is the second of the two written before it: **the part stays
silent with no pin driven**, exactly as it does with both driven. Ten milliseconds, five hundred,
five thousand, pins or no pins: from a plug-in the part never appears on the bus.

**So the pin driving is not what stops it**, and the conclusion of 12:17 is in doubt for a second
reason rather than rescued. That conclusion said fifteen lines driving two pins woke the part. It
was drawn from a pair of runs with two changes in them, and now a run with those lines removed
behaves the same as one with them. **Neither of those is proof that the pins do nothing**, since
no run has yet compared them under conditions where anything answers at all, but the claim as
published is not supported and this page says so.

### What the successes and the failures actually have in common

Reading back every run in this log rather than the last three:

| | How the board started | Pressed RESET | The part |
|---|---|---|---|
| 12:17 to 15:35 | powered for a long time | yes, every time | answers |
| 15:44, 15:50, 15:56, 16:08 | cable out and back in | **no, deliberately** | never answers |

**The variable called "cold" has two things in it**, and the one never isolated is the button.
Every run where the part answered followed a press; every run where it did not, did not. That was
not designed, it fell out of the hand-over, and it went unnoticed for four runs because the
power cycle was the thing being thought about.

This is the same fault as the 12:17 confound and it is the second time today: **a comparison that
moves two things tells you about neither.** The difference is that this time the record noticed
before a conclusion was published, which is the only part of it worth being pleased about.

### The next run isolates the press and nothing else

Cable out and back in as before, then wait, then press RESET once with the console already
capturing. Same image, same shield, same everything.

| What it shows | What follows |
|---|---|
| silent before the press, answers after | the press is the difference, and the question becomes what a reset does that a power-on does not. A supply that is still rising is the obvious candidate and is testable by waiting longer before pressing |
| silent both times | the press is not the difference either, and what changed between 15:35 and 15:44 was something else about the bench, which would be the first thing to establish by eye |

## Saturday 17 October 2026 at 16:39: the board is fine, the part is gone, and what that costs

The board boots, the console works, a press produces two complete runs. **The part has not
answered since 15:44** and nothing tried since has brought it back:

| Varied | Values tried | The part |
|---|---|---|
| the settle bound | 10 ms, 500 ms, 5000 ms | absent |
| the control pins | both driven, neither driven | absent |
| the reset button | pressed, not pressed | absent |
| the shield | reseated and pressed down | absent |

The last run in which it answered was 15:35. Between that and 15:44 one thing happened: **the
board was power cycled for the first time since the shield was fitted.**

### What this costs, stated precisely rather than generally

**The observations stand and the mechanism does not.** The part did answer at 0x29. It did
identify itself as device 0xF0 revision 0x0C. It did refuse the previous generation's firmware
image with a timeout and accept this generation's in 1.971 seconds. It did return sixty-four
distances that formed a coherent surface at two and a half metres. Those happened, they are in
this log with the consoles that produced them, and nothing since casts doubt on any of them.

**What is in doubt is the explanation of why the part was answering at all**, and specifically
the entry of 12:17 which says fifteen lines driving two pins woke it. That reading had a
confound when it was written, and the runs since have shown that with those lines removed the
part behaves no differently. The honest position now: **this project does not know what made the
part answer between 12:17 and 15:35, and does not know what a power cycle changed.**

### Why this stops being a software question here

Everything a description or a driver can vary has been varied. What has not been read is the
expansion board itself: its power path, whether the part has a reset line separate from the two
this project drives, what the current-measurement links on it do, and which positions the
interface jumpers are in. **That is a document and a look at the board, not another build.**

The specific thing to find out is whether this part needs a sequence at power-up that nothing
here performs, and which it happened not to need while it was already running. A part that works
until it loses power and then does not come back under any software is the shape of a missing
step in a power-up sequence, and the shape is a reason to read rather than a reason to conclude.

**One smaller observation, kept because it is odd and unexplained.** Listing the serial ports on
the demo laptop returns COM13 three times, alongside COM3, COM6 and COM7. Three registrations of
one name is not something this bench has seen before. It has no bearing on the part, since the
board talks through that port perfectly well, and it is written down in case it turns out to
matter later.

## Saturday 17 October 2026 at 16:45: the answer was a pulse, and it was in the vendor's own code

ST's application for this expansion board, in the example that ships with their library, under a
comment reading "Sensor reset":

```
PWR_EN low,  wait 2 ms
PWR_EN high, wait 2 ms
LPn    low,  wait 2 ms
LPn    high, wait 2 ms
```

**This project drove both lines high and left them there.** That is the whole fault, and it
explains every observation of the afternoon without needing any of the theories that were tried:

| Observation | What the pulse explains |
|---|---|
| every warm run worked | a part already out of reset does not notice a line that is already high |
| removing the pin driving changed nothing | neither driving them high nor leaving them alone resets anything |
| no cold start ever found the part | the part came up held in reset and was never released |
| ten, five hundred and five thousand milliseconds all failed | waiting does not end a reset |

**And it closes the confound honestly rather than rescuing it.** The entry of 12:17 credited two
pins with waking the part. The pins were not what woke it, because driving them high does
nothing to a part already running, so that reading was wrong and the thing that changed between
11:16 and 12:17 was the shield being seated. The confound was real, it was named here before the
answer was known, and the answer settled it against the published reading.

### How it was found, which is the part worth keeping

Not by another build. Everything a description or a driver could vary had been varied, so the
next step was reading the vendor's own working application for this exact board. The prediction
written before reading it was that the answer would be **either a reset line this project does
not drive at all, or an order and a timing for the two it does.** It is the second, and it is
four lines long.

The two millisecond waits are ST's and are reproduced rather than improved on, which is the same
decision as the ten millisecond settle. The difference is that this time the shape is theirs as
well as the number, and the shape was what mattered.

## Saturday 17 October 2026 at 16:50: the pulse did not bring it back, and the pins are right

The first cold start with the vendor's own sequence: **still no answer at 0x29.** The image was
new, 142884 bytes against 142716, so the pulse was in it; the console's build stamp reads 15:42
for the reason already recorded, since the stamp lives in the application file and this change
was in the driver.

**The pins are not the problem, and that is now checked rather than assumed.** ST's own project
for this shield names `PWR_EN_C` as port A pin 7 and `LPn_C` as port B pin 0 on their board,
which is a different Nucleo. On that board those are the Arduino positions **D11** and **A3**.
This project's overlay names the same two Arduino positions, which this board maps to PA7 and
PB1. So the description has been right all along and it is right for a reader with a third board
as well, which is what naming positions instead of ports was for.

### What has still never been confirmed, after four runs that rested on it

**That a pin is driven at all.** Every entry since 12:17 has reasoned about pin driving, and no
console has ever said a pin was driven. The driver's own helper returns success and says nothing
both when it pulses a line and when the description does not carry one, so those two cases are
indistinguishable in every log this project has. The helper now reports what it did, which costs
nothing and should have been there from the first version.

**And what the shield physically looks like.** It was asked for at 16:26 and the answer came as a
command rather than a description, so the record still does not say whether the board is seated
flat, whether anything is trapped under it, or where its jumpers sit. Four software experiments
have been run on a bench whose state is written down nowhere since the morning. That is the rule
this project has on file and has now failed twice in one afternoon: **a document records what was
built, not what is connected now.**

## Sunday 18 October 2026 at 19:21: the pins are driven, and the software is now exhausted

**Prediction written before the run:** two new lines naming a port and a pin each. If they appear
and the part still does not answer, the software has been exhausted and the remaining question is
the shield itself.

Both halves came true. The console:

    <inf> vl53l8cx: supply enable pulsed low then high on gpio@58020000 pin 7, 2 ms each
    <inf> vl53l8cx: low power pulsed low then high on gpio@58020400 pin 1, 2 ms each

`gpio@58020000` pin 7 is PA7, which this board puts at Arduino D11. `gpio@58020400` pin 1 is PB1,
at Arduino A3. Those are ST's two pins at ST's two positions, pulsed in ST's order with ST's two
millisecond waits. **After four runs that reasoned about pin driving, a console has finally said a
pin was driven.**

### The log's words are true of the pins, which is a second thing and was also checked

A helper that reports "low then high" is reporting the logical levels it asked for, and whether
those are the physical levels depends on a flag in the description. Both properties carry
`GPIO_ACTIVE_HIGH`, in both overlays, so logical and physical agree and the sentence on the
console is a sentence about the pin. Had that flag said otherwise, the log would have read
correctly while the supply was being held off, which is the failure this check exists to exclude.

### Why the scan's answer rules out the driver rather than implicating it

**Nothing answered, 0 of 112 addresses.** That is neither of the two failures a driver can
produce, and the scan was built to separate them:

    a wrong address        something answers somewhere, and the scan prints where
    a scan that is void    everything answers, and the scan says so rather than reporting 112
    nothing at all         the part is not electrically on this bus, which no address, no bus
                           speed and no register sequence can change

Bus speed was already eliminated by the 100 kHz control. The address is the one the part itself
used this afternoon. So the result is not about software, and four more builds would not move it.

### What makes this a regression rather than a bring-up problem

Earlier the same day, this same description and this same driver read a coherent grid at about
2.56 metres with a 52 millimetre gradient from left to right, and a silicon temperature. The
part, the bus wiring and these two pins were all sound a few hours ago. The only event between
that reading and the silence is the power cycle at 15:44, cable out and back in. **A working
system did not become a broken driver by being unplugged.**

### The stamp defect caught itself in the act, and is fixed

This console reads `# built Oct 17 2026 15:42:20` on an image that demonstrably contains code
written after 16:50, since the two lines above are in it. The stamp was `__DATE__` and `__TIME__`
in the application file, which fix themselves when that one file is compiled and not when the
image is linked. It has been replaced: `scripts/stamp.py` runs at every build and writes the time
in full and the commit, with a count of the files in this project that were edited or untracked
at that moment. The first version of that script used `git describe --dirty`, which reads tracked
modifications only; a tree carrying an untracked C file was proved to report a clean hash, so the
marker is computed instead. Both directions were run before the script was committed.

The stamp was added so a stale flash could be told from a reproduction, and in this form it did
the opposite: it reported a new image as an old one. **A guard that produces false alarms is worse
than no guard, because the result it discards is real.**

### The one thing still not examined, now the only thing left

The shield's physical state, asked for at 16:26 on Saturday 17 October 2026 and again at the end
of that day. Five software experiments have now been run on a bench whose state nothing records
since Saturday morning. What is wanted is not another build but a description or a photograph:
whether the board sits flat on both header rows with no tilt or gap, whether anything is trapped
beneath it, and where its jumper blocks sit, particularly the pair that selects between the
two-wire and four-wire interfaces. A part that answers for hours and then does not, across one
cable cycle, is a question about contact and configuration.

## Sunday 18 October 2026 at 19:50: two questions answered away from the bench

Neither of these needed the board, and both were open while the shield question waits.

### The status values, as far as a source will say

The grid printed bare numbers because this project would not invent a meaning for them. The
vendor's header, at the commit `CMakeLists.txt` pins, says exactly one thing about them, beside
the `target_status` field:

    Status indicating the measurement validity (5 & 9 means ranging OK)

That is enough for the second acceptance criterion, which is about telling a measurement from a
number, and it is now in the printer: each zone carries an asterisk when its status is 5 or 9,
and the count of marked zones is printed under the grid.

**The rest of the enumeration is still not established, and still not guessed.** The whole pinned
package carries one documentation file, `Documentation/X_CUBE_TOF1.chm`, which is compiled help,
and no source file in it lists the values. So an unmarked zone is reported as a number with no
interpretation, which is what the project said it would do.

### The data-ready line is at Arduino A2, which is PC3 on this board

The fourth criterion wants the line the part raises when a frame is ready, and the position was
unknown. ST's pin configuration for this shield gives `TOF_INT_EXTI` as port A pin 4 on their
board, a different Nucleo, so the position has to be resolved through that board's connector map
rather than read off the port name.

**That map was checked against two facts already confirmed on this bench, so the lookup could
fail rather than merely agree.** On ST's board it gives D11 as port A pin 7 and A3 as port B pin
0, which are precisely their `PWR_EN_C` and `LPn_C`. With the method confirmed on both anchors,
their interrupt pin resolves to Arduino **A2**.

This board's own map then gives, and the first two match what the driver printed at 19:21 to the
port and the pin:

| Arduino position | ST's board | This board | What it carries |
|---|---|---|---|
| D11 | port A pin 7 | port A pin 7 | supply enable, confirmed driven |
| A3 | port B pin 0 | port B pin 1 | low power, confirmed driven |
| A2 | port A pin 4 | port C pin 3 | data ready, not yet described |

The two boards disagree about A3 and agree about D11, which is the argument for naming positions
rather than ports made by the hardware instead of by a design document.

### The overlay is deliberately not changed yet

The binding already declares `int-gpios` optional, so adding the line is one edit. **It is being
held.** There is an unexplained regression open, and the description is the thing under test: a
new property in it would be a second variable in exactly the way that produced the confound of
12:17, when two things moved between two runs and the result was credited to one of them. The
line goes in once the part answers again, and not before.

## Sunday 18 October 2026 at 20:30: the vendor's valid statuses contradict the only grid we have

**Caught by reading this log rather than by a run, and it would have arrived on the bench looking
like a broken printer.**

The marking committed an hour earlier takes the vendor's header at its word: statuses 5 and 9 mean
ranging OK. The entry of Saturday 17 October 2026 at 15:35, the only grid this project has ever
recorded, says of the coherent surface at about 2.56 metres:

    every zone reporting one target, every status 6

And of the noise grid from the boot before it, the same: status 6 everywhere. So under the
vendor's own sentence, **the one real measurement this project has taken would have been marked as
zero valid zones out of sixty-four**, and the line printed underneath would have told a reader
those were not distances the part measured.

That line has been corrected. The count stays, because it is what the source says; the claim that
an unmarked zone is not a measurement is withdrawn, because this log does not support it.

### Two readings, and this project cannot choose from here

    6 is a state the parenthesis       the measurement is sound and the header's list is
    does not list                      incomplete rather than wrong. Early frames after a start
                                       are a candidate, but naming the state would be a guess
    the parenthesis is about           some other field, or the generation before this one, in
    something else                     which case the quotation is true and irrelevant here

Both outrank a mapping invented to make the output agree with itself. The conflict is now printed
beside the count, so the next run reports it rather than implying a fault.

### What this does to the second acceptance criterion

The criterion asks that an empty scene and a surface beyond range be distinguishable by status
rather than by distance alone, and the chapter says the rest of the volume depends on it. **In the
only dataset that exists the status is uniform and distinguishes nothing**: 6 across a coherent
surface and 6 across sixty-four zones of noise reading between -2 and +5 millimetres, ten of them
negative. What separated those two grids was the distances and the coherence, which is precisely
the reasoning the criterion was written to avoid having to do.

So the criterion is not met, and more usefully it is not yet posable: it needs a frame in which
the statuses are not all one value. Recorded here rather than in the chapter's criteria table as a
pass, which is what reading the old table would have produced.

### The method note, since this is the second time today

Both of today's two defects were found by reading a record instead of running something. The stamp
reported a new image as stale, and this marking would have reported a real surface as no
measurement at all. **Each was a guard whose failure direction was to discard good evidence**, and
in both cases the guard was written from a source that was correct and incomplete. A guard earns
its place by being tried against the data already on file before it is pointed at new data.

## Sunday 18 October 2026 at 21:40: the pin was a comment, which is not what this file claimed

**Criterion 7 needs a host and no hardware, so it was the one piece of work available while the
shield question waits.** Writing the check found that the thing it was meant to confirm was false.

This project's build file carried two sentences about the vendor's revision: that the pin "is what
was built against and not what was intended", and that "the version becomes part of the build
rather than part of the history". The revision appeared in exactly one place in that file:

    #   git -C ~/src/x-cube-tof1 checkout c12709c3ac802d91dd2a215178f53cb94c024709

A comment. No cmake command read it. **The build compiled whatever revision happened to be checked
out in the clone**, so a single `git pull` in that directory would have moved every figure this
project publishes without changing a line here, and nothing would have said so. The claim was a
claim about a comment.

### What now enforces it, in two places because they catch different things

    at configure time   the build asks the clone what revision it is at, with git rev-parse HEAD,
                        and refuses to configure against any other. Deliberately fatal, since
                        every published figure is attributed to the pinned one, with an explicit
                        -DVL53L8CX_ULD_ANY_REVISION=ON that warns instead and says the build is
                        unpinned
    in the tree         scripts/check_vendoring.py, which runs anywhere and needs no clone: the
                        revision must be a cmake value and a full forty characters, the source
                        path must not default inside this repository, and every C and C++ file
                        under projects/ must name its own path

### The third rule, and why it is a house-style check rather than a search for vendor names

A list of third-party filenames goes stale and says nothing about the next library. What
generalises is that **every C and C++ source in this tree introduces itself by its own repository
path on its first line**, which holds for 61 of 61 such files. A file carrying somebody else's
copyright header does not, so a vendored drop fails on line one whatever it is called. Rust is
excluded deliberately: those ten files carry `//!` module documentation describing their role,
which is that language's convention and was read before being called a defect.

### All four failure cases were built on purpose

A gate that has only ever passed is a gate nobody has checked, so each was constructed and watched
being detected, with the file restored from its original bytes afterwards:

| Mutation | Detected |
|---|---|
| a branch name where a commit belongs | yes, and it reported the comment rule as well |
| the revision demoted back to a comment | yes |
| an external source defaulting inside this repository | yes |
| a source file that does not introduce itself | yes |

The second case first reported "mutation did not apply", which is the dangerous outcome: it reads
as a passing gate. The needle was written with one line ending and this file uses the other, so it
matched nothing. **A mutation that does not apply proves neither direction**, and the harness now
normalises before matching and says so when a mutation is inert.

## Sunday 18 October 2026 at 22:35: the suite is real and the proof it could fail was not

**The reduction compiled and passed on its first run on the demo laptop**, which is not what was
predicted. The hand-over said to expect that step to find something, since the file was C that no
compiler had ever seen, built with warnings as errors. It came back clean: 32 checks, 0 failures,
across nine cases including the two refusals that matter.

Then `make mutants` printed `all 4 mutations were caught, so the suite can fail`, **and that line
was false.**

### How a green line with a zero exit status was worth nothing

The four mutations were a `mutant-%` pattern rule, and the four targets were declared `.PHONY`.
**GNU make does not search pattern or implicit rules for a phony target.** So all four became
phony targets with no recipe at all: make had nothing to do for each, did it silently, and the
summary in the parent target then announced the result it was written to announce.

Nothing failed. Nothing ran. The exit status was zero and the words were encouraging.

**The only tell was four missing lines.** Each mutation was supposed to print its own name and how
many checks it broke, and the pasted output went straight from the suite's last line to the
summary. A run that produces less output than it should is a run worth reading twice, and this one
would have passed unexamined if the per-mutation reporting had never been written in the first
place.

### This is the second version of this target to report success having done nothing

The first looped in the shell over the mutation names and asked for `$SED_$m`, which the shell has
never heard of, so every expression was empty and every mutation a copy of the original. That one
was caught by reading it. This one was caught by its own output being too short.

| Version | What it did | How it was caught |
|---|---|---|
| shell loop over make variables | every sed expression empty | read before it ran |
| pattern rule with phony targets | no recipe matched, nothing ran | four lines missing from a green run |
| a stamp file per mutation | make must produce a file, and the count is asserted | the subject of the next run |

### The fix is to make the work leave evidence rather than to trust its silence

Each mutation now produces `build/caught_<name>`, and the summary counts the stamps and refuses
unless there is one per mutation. A recipe that does not run leaves no stamp. **Make's own
machinery is the proof the work happened**, instead of the absence of a failure, which is what
both earlier versions mistook for success.

The method note is the one this project keeps relearning in new costumes: a guard, a counter, a
gate and now a mutation harness have each reported a pass while measuring nothing. The shape is
always the same, and so is the remedy: build the failure the thing claims to detect and watch it
be detected. Here that means `make mutants` must be shown to fail when a mutation is removed from
the list, and that run has not been made yet.

## Sunday 18 October 2026 at 22:55: the harness runs, and my test of it was void

The third version works. Four mutations, each with its own line, each with a plausible count of
broken checks:

    accept_everything: caught, 9 check(s) failed
    ties_to_higher: caught, 1 check(s) failed
    never_initialises: caught, 16 check(s) failed
    always_found: caught, 14 check(s) failed

    all 4 mutations ran and were caught, so the suite can fail

### The test I handed over to prove the stamp count works proved nothing

It ran `make mutants MUTANTS="accept_everything ties_to_higher never_initialises"` and expected a
refusal, on the reasoning that three stamps against four expected would fire the guard. **Overriding
that variable changes the work and the expectation together.** The target made three stamps and
compared them against `$(words $(MUTANTS))`, which was now three, so it reported three of three and
was right to. The run printed my own error message.

This is the same mistake as the Zephyr configure of Saturday 17 October 2026, where a control
reused the build directory under test. **A control that moves the thing under test is not a
control**, and twice now the giveaway has been that the experiment could not have failed.

### The test that does work, because it reintroduces the exact defect

The guard exists to catch one thing: a prerequisite that make silently does nothing for. So put
that back without touching the tracked file, by including the Makefile and re-declaring the stamp
targets phony:

    printf 'include Makefile\n.PHONY: $(foreach m,$(MUTANTS),$(BUILD)/caught_$(m))\n' > /tmp/phony.mk
    make -f /tmp/phony.mk clean && make -f /tmp/phony.mk mutants

That must report `only 0 of 4 mutations left a stamp, so this proves nothing`, which is the
sentence that would have caught the defect of an hour earlier. Anything else means the guard is
decoration.

### One number in that output is worth keeping

`ties_to_higher` broke **exactly one check** where the others broke nine, sixteen and fourteen.
That one check is the tie case, and it is the only thing in the suite covering tie ordering. A
mutation caught by a single check names a load-bearing check: deleting it as redundant would
remove the only cover for the rule that a tie keeps the lower zone index, and nothing else in the
suite would notice. The counts are therefore not decoration either, they say which checks are
carrying weight alone.

## Sunday 18 October 2026 at 23:20: the chain under the reduction is closed

Three targets, three results, and each one is load-bearing for the one above it:

    make check      32 checks, 0 failures, across nine cases
    make mutants    accept_everything 9, ties_to_higher 1, never_initialises 16,
                    always_found 14 checks broken. All four ran and were caught
    make selftest   with the historical .PHONY defect reintroduced into the makefile the work is
                    done by: "only 0 of 4 mutations left a stamp, so this proves nothing"

**That is a closed chain rather than three separate assurances.** The suite passing says the
reduction behaves. The mutations say the suite can fail, so its passing means something. The
selftest says the mutation result cannot be a silent no-op, which it twice was. Each layer has
something underneath it that would notice if it stopped working, and the bottom layer tests itself
in continuous integration rather than when somebody remembers.

It took three versions of the harness and two void tests of the guard to get there, all recorded
above. The cost was worth paying once; the thing that makes it worth paying is that none of it is
now a matter of memory.

## What the sixth criterion still needs, read rather than guessed

The half that needed writing is written. The remaining half is that P01's application object is
byte for byte what it was with only its range source different, and **the tool for it already
exists**: `scripts/check_object_identity.py`, written for P03's first criterion, which compares
two build directories by disassembly and symbol table and tolerates exactly one class of
difference, the `__device_dts_ord_` pair that a devicetree ordinal moves.

Two things stand between here and that check, and neither is a guess:

- **The checker names one object, `CMakeFiles/app.dir/src/main.c.obj`, as a constant.** Criterion 6
  is about P01's presence machine rather than an application's `main.c`, so that path has to become
  an argument. That is a small change and it strengthens the script for every later chapter.
- **The comparison needs two builds of P01 and the second does not exist.** P01's machine is fed
  events by an adapter; nothing yet feeds it from this part through the reduction. That integration
  is the real work, it needs the Zephyr software development kit rather than the board, and it is
  the right place for a session to start rather than the place for one to end.

Recording it here in that order so the next session reads the sibling before writing the copy,
which is the rule this project has already been saved by twice.

## Tuesday 20 October 2026 at 11:58: the part answers again, and the photographs exist

**The photographs were taken before power was applied and before anything was touched**, which is
the rule written the day before. Two views, both held by Joseph.

### What the photographs show

From above: the Raspberry Pi 4B with the MCC 118 on its header, and the NUCLEO-H7A3ZI-Q with the
X-NUCLEO-53L8A1 stacked directly on its Arduino headers, no jumper leads on the shield. One blue
lead runs from the MCC 118's green screw terminal block, at the CH0 end, down to the Nucleo.

**Where that lead enters the Nucleo is the fact the record wanted.** It goes into the long double
row of male pins along the Nucleo's top edge, the Morpho connector CN11, at its left third. That
is outside the shield's footprint. **So the hypothesis named on Saturday morning, a ground lead
occupying a Zio socket the shield seats into, is not supported by the pictures.** The lead was
never under the shield.

On the shield: the three current-measurement shunts at VDD, VDD and 1V8 are fitted, and jumpers
sit at J7, J8 and J9. The side view was taken from above the table rather than at it, so flatness
cannot be judged from the picture; the run below judges it instead.

### The run, and why it is confounded

    <inf> vl53l8cx: supply enable pulsed low then high on gpio@58020000 pin 7, 2 ms each
    <inf> vl53l8cx: low power pulsed low then high on gpio@58020400 pin 1, 2 ms each
    <inf> vl53l8cx: device 0xf0 revision 0x0c at 0x29
    #   0x29 answers, which is the address the overlay names
    READY vl53l8cx@29
    <inf> vl53l8cx: initialised with this generation's own interface
    <inf> vl53l8cx: ranging started at eight by eight

Four frames, every zone one target, a flat surface at about 2.58 metres with the same left to
right rise as on Saturday 17 October 2026, silicon at 23 then 22 degrees. Initialisation at 1.979
seconds, which is within a hundredth of the 1.971 recorded before.

**What brought it back cannot be said, and the record says so.** Between the last silent run and
this one the bench was handled to photograph it: the two pictures show the boards in different
positions relative to each other, so the shield may have been reseated by the handling, or not.
This is the same shape as the 12:17 confound and it is named rather than resolved. What is not in
doubt is that the configuration in the photographs is a working one, since it was photographed
before power and then powered.

### The image that ran was the previous day's, and the stamp could not say so

The WSL build succeeded at 144344 bytes and the copy to the Windows desktop failed:

    cp: cannot stat '/mnt/c/Users/skyhorizon/Desktop/p05-own.bin': Input/output error

So the file flashed was the one already on the desktop, from the build of Saturday evening.
The console confirms it in every line: the stamp is the old `built Oct 17 2026 15:42:20` form, the
grid header still reads "what each value means has not been read from a source yet", there are no
validity marks, and no reduction is printed. **The one run in which the generated stamp could have
shown its worth did not contain it.** Nothing above is invalidated by this; the part, the pins, the
bus and the initialisation are the same code. What is untested on hardware is everything added on
Sunday 18 October 2026.

### The status values, which this run answers and the next must confirm

| Frame | Ready after | Since boot | Status, all sixty-four zones |
|---|---|---|---|
| 1 | 29 ms | 2448 ms | **6** |
| 2 | 622 ms | 3169 ms | **5** |
| 3 | 877 ms | 4145 ms | 5 |
| 4 | 877 ms | 5121 ms | 5 |

**Status 6 is the first frame after ranging starts, and status 5 is every frame after it.** The
vendor's header says 5 and 9 mean ranging OK, and that is now consistent with data: the only grid
recorded before this was a first frame, which is why it was 6 everywhere, and so was the noise
grid before it. The reading this project could not choose between on Sunday, that 6 is a sound
state the parenthesis does not list, is the one the data supports. The other reading, that the
parenthesis is about a different field, is not.

**Prediction for the next run, written before it:** with the current image, frame 1 reports
status 6 in every zone and the reduction prints NO READING with 0 of 64 valid; frames 2 to 4
report status 5 in every zone, carry an asterisk on every zone, and the reduction prints a
distance from the nearest zone with 64 of 64 valid. The stamp reads a full date and the commit.
If frame 1 is not 6, or any later frame is not 5, the pattern above is one observation and not a
mechanism, and `zone_status.h` is not changed.

### The ranging period is now a measured number

Frames 2, 3 and 4 became ready at 3169, 4145 and 5121 milliseconds since boot: **976 ms between
frames, twice**, which with 5 ms polling granularity is a ranging rate of about 1 Hz. The chapter's
timing figure was left with "length not established" on Sunday rather than a guessed 66 ms, and
this is what goes in its place once the next run repeats it.

## Tuesday 20 October 2026 at 12:07: the prediction holds, twice, and the stamp proves itself

Between 11:58 and this run the Windows drive disappeared from WSL: `ls -ld /mnt/c/Users/skyhorizon/Desktop`
itself returned input/output error 5, which is why both earlier copies had failed and the stale
image had been flashed twice. The image went to the board the other way, read from WSL over
`\\wsl.localhost\` by PowerShell and written straight to the board's drive, 144344 bytes, with the
size printed before anything was run.

**The stamp, on hardware, for the first time:**

    # built Tuesday 20 October 2026, 11:57:44
    # from f2cdf51, and a stamp older than your last edit means a stale flash

A full date and a commit, as designed on Sunday, and the stale runs of 11:58 and 12:01 are
exactly the case it exists for: both read `Oct 17 2026 15:42:20`, and that was true of them.

### Every element of the written prediction, against the run

| Predicted before the run | Observed, boot one | Observed, boot two |
|---|---|---|
| frame 1 status 6, no marks, `NO READING, 0 of 64` | yes | yes |
| frames 2 to 4 status 5, 64 of 64 marked | yes | yes |
| a distance from the nearest zone, near 2550 mm, left columns | 2545 mm zone 10; 2551 zone 24; 2551 zone 17 | 2547 mm zone 9; and the same two after |
| stamp a full date and the commit | yes | yes |

Zones 9, 10, 17 and 24 are rows 1 to 3, columns 0 to 2 in the order the part returns them, which
is the near side of the same left-to-right rise seen on every grid since Saturday.

**That is five boots out of five with status 6 on the first frame and status 5 on every frame
after, three of them after the prediction was written.** The reading this project could not choose
between on Sunday is chosen by the data: 6 is the state of the first frame after ranging starts,
and the vendor's 5 and 9 are consistent with every frame from the second on.

**And the first frame is not to be trusted, which makes the refusal protective.** The noise grid
of Saturday 17 October 2026, sixty-four zones reading between minus two and plus five millimetres,
was also status 6. A first frame can be a coherent surface, as above, or garbage, as then, and the
status does not distinguish those two; what it does is mark both as not a reading, which is the
vendor's rule and now the right one. `reduce_nearest_valid` giving P01 nothing on frame 1 is the
behaviour wanted.

One zone is worth a note and no conclusion: in frame 2 of the second boot, the bottom right zone
read 2711 mm against about 2600 around it, status 5. A single outlier in one frame of one boot.

### Measured, replacing figures the chapter left blank on Sunday

| Quantity | Value | From |
|---|---|---|
| ranging period, vendor default at eight by eight | 976 to 982 ms, about 1 Hz | frames 2, 3 and 4 of both boots, and of the 11:58 and 12:01 runs |
| initialisation, image upload included | 1.981 s | both boots, against 1.979 and 1.971 before |
| first frame after start | 2453 ms since boot, 29 ms of waiting | both boots |
| silicon temperature | 26 C on the first frame, 22 C after | both boots |

### Prediction for the orientation run, criterion 5, written before it

With the bench untouched and a hand held about twenty centimetres above the sensor so that it
covers roughly the quarter of the sensor's view nearest the blue USER button, frames 2 to 4 will
show **one four by four block** of zones at a few hundred millimetres with status 5, the other
forty-eight near 2580, and the reduction will name a zone inside that block. Which printed corner
that block occupies is the orientation, and it is the thing not predicted, since predicting it is
what the run is for. If more than one block moves, the hand is too close or too large for the
field and the run is repeated further away rather than interpreted.

## Tuesday 20 October 2026 at 12:18 and 12:21: a hand over the sensor, too close to read a quadrant

Two runs, each with a hand over part of the sensor's view before reset and held through the four
frames. The first with the hand towards the blue USER button, the second towards the black RESET
button; both logs carry the first name because the command was reused, and the order is taken
from the times. **The hand was about 30 millimetres from the sensor in both, not the 200 asked
for**, and the prediction said what that would do: more than one four by four block moves, and
the run is repeated rather than interpreted. So the orientation is not read from these.

### What the two runs did establish, which is not nothing

**The status field has a vocabulary and it tracks the scene.** Four days of grids had shown two
values, 6 on a first frame and 5 after. With a hand in the field the frames carried 4, 5, 6, 10,
13, 14 and 255. The one that can be read from the data alone is 255: in every zone that reported
it, the targets grid reported zero, and in no other zone. **Status 255 is a zone that saw no
target**, and the distance printed beside it, around 30 millimetres, is a number and not a
measurement. That is the second acceptance criterion observed rather than argued: an empty zone
and a measured zone both carry a distance, and only the status and the target count tell them
apart. The other new values are not named here; the vendor's enumeration is still in a compiled
help file.

**The reduction behaved under a real near object.** With a hand at 30 millimetres in half the
field, it reported the nearest zone the vendor calls a reading, 26 to 30 millimetres, and passed
over every zone at 255, 10, 4 or 13 beside it, including ones with smaller numbers in them. `27 mm
from zone 58, with 58 of 64 zones valid` is the sentence P01 would have been given, and it is
right.

**Frame 1 is status 6 on the uncovered zones and 13 on the covered ones**, in both runs, which is
a sixth and seventh boot agreeing that the first frame is never 5.

### What the shape of the covered zones suggests, as a hypothesis for the repeat and not a result

| Hand towards | Covered in the printed grid |
|---|---|
| the blue USER button, at the board's left end, CN11 side | columns 4 to 7 in every row, spreading to column 3 in the bottom two rows |
| the black RESET button, at the board's left end, CN12 side | column 0 in every row, and columns 0 to 3 in the bottom two rows |

Both hands came in over the board's left end, and both runs show the bottom two printed rows
covered; what differs between them, the CN11 side against the CN12 side, shows as printed right
against printed left. **If that holds at a proper distance, the printed grid is rotated a quarter
turn relative to the board**: the board's left end is the bottom of the grid, and the CN11 edge
is the grid's right. Written here so the repeat can refute it, which a hand at 200 millimetres
covering one clean block either will or will not.

## Tuesday 20 October 2026 at 12:27 and 12:28: one finger, one zone, one reference point

A single index finger over the field instead of a hand, held through four frames.

### Towards the blue USER button: exactly one zone, four frames running

| Frame | Zone 56, row 7 column 0 | Every other zone | Reduction |
|---|---|---|---|
| 1 | 426 mm, status 6 | about 2580, status 6 | no reading, as always on frame 1 |
| 2 | 436 mm, status 5 | about 2580, status 5 | `436 mm from zone 56, 64 of 64 valid` |
| 3 | 427 mm, status 5 | the same | `427 mm from zone 56` |
| 4 | 438 mm, status 5 | the same | `438 mm from zone 56` |

Sixty-three zones unchanged and one zone at the finger, in the order the part returns them the
bottom left corner of the printed grid. The finger was at about 43 centimetres rather than the 10
asked for, which is why it covered one zone and not six, and that is the distance to keep.

**The hypothesis from the hand runs is refuted on its column half.** It said USER would land at
the printed bottom right; it landed at the bottom left. The row half, that the board's left end is
the bottom of the grid, survives this run but is not confirmed by it alone. The hand runs at 30
millimetres were the wrong instrument and this is the right one, which is what the prediction said
would decide between them.

### Towards the black RESET button: no zone moved

Four frames of the flat surface, 64 of 64 valid, the reduction picking far zones 24, 16 and 24 at
about 2545 millimetres as on an empty field. One zone in frame 2 carried status 9, the vendor's
other ranging OK value, seen here for the first time. The finger was outside the sensor's view:
a null result about where the field is, not a result about that corner.

### What one point fixes, and the two outcomes the next run chooses between

With the board's long axis called X, positive towards the ST-LINK cable, and the across axis Y,
positive towards the Pi, the USER direction is roughly minus X plus Y, and it landed at row 7,
column 0. Two symmetries of the square send that direction there, and one more direction tells
them apart:

| Finger towards | If minus X is row plus and plus Y is column minus | If minus X is column minus and plus Y is row plus |
|---|---|---|
| the ST-LINK cable, plus X | the **top** rows, middle columns | the **right** columns, middle rows |
| the Pi, plus Y | the **left** columns, middle rows | the **bottom** rows, middle columns |

Either run alone decides; both together cross-check. The firmware is left exactly as it is through
these runs, including the printed sentence that now says the status question is unresolved, so
that nothing changes between measurements but the finger.

## Tuesday 20 October 2026 at 12:37 and 12:38: the finger protocol cannot place a finger

Two more finger runs, towards the ST-LINK cable and towards the Pi, at the 43 centimetres that had
given one clean zone.

| Finger towards | What moved |
|---|---|
| the ST-LINK cable | a line in **column 0**, rows 1 to 6, at 367 to 421 mm: a finger seen lengthwise |
| the Pi | nothing, 64 of 64 at the surface: outside the field |

### Four runs, two hits in the same column, two misses, and what that says

| Run | Direction asked | Result |
|---|---|---|
| 12:27 | USER button | one zone, row 7 column 0 |
| 12:28 | RESET button | nothing |
| 12:37 | ST-LINK cable | a line, rows 1 to 6, column 0 |
| 12:38 | Pi | nothing |

Two directions that differ by something like ninety degrees both landed in column 0, and two
others landed nowhere. **A finger at arm's length, 43 centimetres up, aimed by eye at a feature on
the board, cannot be placed to within the five centimetres a zone subtends at that height.** The
two hits are consistent with the hand entering the field from the same side each time, and the
misses with it being past the edge of a cone that is only about eighteen centimetres wide there.
The prediction table of 12:28 offered two outcomes for the cable run; the result matched neither,
which is the table being honest rather than the sensor being odd.

**So nothing is read from column 0.** The orientation remains unestablished and the record says
so. What these runs did add: status 4 appeared on the boundary zones of the finger in three
frames, beside 5 on its body, which is a third value seen at an edge after 10 and 255 on Sunday's
hand runs.

### The protocol that replaces it, written before it is run

The error is the standoff. An object anchored at the sensor cannot miss and cannot be misplaced:

- **A pen held flat about 15 centimetres above the sensor window, one end directly over the
  window, the pen extending towards the ST-LINK cable.** The sensor should see a line from the
  centre of the printed grid to one edge. That edge is the cable's direction.
- **The same, the pen extending towards the Pi.** A line from the centre to a different edge, at
  right angles to the first. That edge is the Pi's direction.

Two edges at right angles fix the orientation completely, and a line that reaches an edge from the
centre cannot be confused with a line that happens to lie along one. If either run shows the pen
along an edge rather than from the centre, the pen's near end was not over the window and the run
is repeated rather than read.
