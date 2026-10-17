# P05 design: the next generation of a part the tree already drives

Written on Saturday 17 October 2026, **before any code in this project exists**, which is the
requirement every project in this volume starts with and the reason the git history is the
evidence rather than this sentence. Every statement below about a measurement is a requirement
on work not yet done.

## The claim, and why it is worth a chapter

Everything in this volume that decides whether a room is in use rests on a sensor that can tell
a person from a chair. The part on this bench that does that resolves an eight by eight grid of
distances, and the question it answers is not how far away something is but how much of a room
is occupied and roughly where.

The chapter exists because the operating system's tree does not carry a driver for this part. It
carries one for the generation before it, which is the finding that reshaped this page and is
recorded below with the command that produced it. So the work is not to introduce a family the
tree has never met. It is **to bring the next generation of a part into a tree that already
drives the previous one**, which is the situation an engineer actually meets and the harder of
the two to do well.

The value of that is not the part. It is that the application above it cannot tell this device
from any other: [P01](../../01-presence/) keeps its state machine and only its range source
changes, which is criterion 6 and is the one criterion that can be checked byte for byte.

## The first thing to do, and it is a check rather than a line of code

**The chapter asserts an absence: that this part has no driver in the tree.** An absence is
checkable, the check costs one command, and a chapter that asserted it on the strength of
somebody's memory would be asserting the one thing that makes the rest of the work necessary.
So before a binding is written, in WSL on the demo laptop:

    ls ~/zephyrproject/zephyr/drivers/sensor | grep -i vl53
    grep -rn "vl53" ~/zephyrproject/zephyr/dts/bindings/sensor/ | head

The pinned tree is 4.5.0-rc1, which is what the P04 board runs printed on Saturday 17 October
2026. **If a driver for this exact part is there, this chapter changes shape and says so**: the
work becomes a binding and an application over an in-tree driver, the out-of-tree module
argument goes, and the chapter is shorter and more honest than it was. A near neighbour is not
the same thing: a driver for a single-point part in the same family is not a driver for a
sixty-four zone one, and the difference has to be read in the binding rather than inferred from
the name.

### What that command returned, Saturday 17 October 2026, and why the premise narrows

**The paragraph above was written before the command was run.** It is kept exactly as it was,
because the answer landed between writing it and reading it, and because it turned out to have
anticipated the wrong half of the question.

**The tree carries bindings for `st,vl53l0x`, `st,vl53l1x` and `st,vl53l5cx`, and none for the
VL53L8CX.** So the chapter's assertion is true of this exact part and **misleading as written**.
The VL53L5CX is not a distant relative and not the single-point neighbour the paragraph above
guarded against: it is the previous generation of the same multizone family, with the same eight
by eight grid. "A part the tree does not know" invites a reader to picture a family the tree has
never heard of, and that is not the situation.

The narrower and more useful statement, which is what this project is now about: **the tree knows
the previous generation of this part and not this one, and the interesting question is what
differs between them.** That is a better chapter than the one written, because the gap is
specific and a reader can check it.

Two things follow and both are open until the next command runs. **First, the in-tree driver for
the previous generation may already drive this part**, or come close enough that the work is a
binding and a compatible string rather than a driver. That has to be established and not
dismissed, because a cheap route left unexplored is the kind of thing a reader of a portfolio
notices. **Second, if the in-tree driver pulls the vendor's own library in as a module**, then
the module arrangement this chapter sets out to design already has a worked precedent inside the
tree, and the chapter should follow it rather than invent a second one.

One method note, because it nearly became a wrong conclusion. **`ls drivers/sensor` matched
nothing for this family**, and on its own that reads as the absence the chapter claimed. The
bindings are what proved the drivers exist, so the drivers are not flat names in that directory
in this revision: they sit under `drivers/sensor/st/`. An empty listing is not an absence until
you know you looked in the right place.

### What the second command returned, and the deliverable changes

Three things are now established and one is not.

**Nothing anywhere names the VL53L8CX.** A case-insensitive search of the whole operating system
tree and of every module beside it returns no file. So for this exact part the chapter's premise
is exactly right: there is no binding, no driver, and no vendor library already in the workspace.

**The driver for the previous generation is substantial and modern.** Nine files under
`drivers/sensor/st/vl53l5cx/`, including a decoder and, in `vl53l5cx_rtio.c`, support for the
read-and-decode interface rather than only the older fetch-and-get one. **That is the shape this
project has to match**, and matching it is more work than vendoring a driver and more useful:
a part whose driver ignores the interface the tree has moved to is a part that will be rewritten
by somebody else within a year.

**The in-tree binding declares neither a reset pin nor a data-ready line.** It is eleven lines:
a compatible string and `include: [sensor-device.yaml, i2c-device.yaml]`, with no GPIO property
of its own. **That matters for criterion 4**, which asks that the application never polls and
needs the trigger connected. So the binding this project writes is not a copy of the in-tree one
with a name changed: it declares two facts about the wiring that the in-tree description leaves
out, and the chapter has to say why it is right to add them rather than follow the neighbour.

**Not established: whether the vendor's own library for the previous generation is already in the
workspace** as part of the hardware abstraction module. The search above was for the L8 and found
nothing, which says nothing about the L5. It decides where this project's vendor source comes
from, so it is the last check before code.

### So the deliverable is not the one the chapter wrote

The chapter's deliverable reads: a binding, a driver and a module that let the sixty-four zone
sensor be used through the same interface as a three-axis accelerometer, with the vendor's code
vendored rather than rewritten. **The second half of that is now the wrong instinct.** Vendoring
a parallel copy of a vendor driver into a repository whose operating system already drives the
previous generation of the same part, in-tree, against a newer interface, would be building a
second way to do something the tree already does one way.

The revised deliverable, and the reason it is a better chapter:

| | What |
|---|---|
| The comparison | What differs between the two generations, read from the vendor's own library for each and not from marketing material. This is the chapter's real content and it did not exist in the original plan |
| The binding | For this part, declaring the reset pin and the data-ready line that the in-tree neighbour omits, with the omission named rather than silently corrected |
| The driver | Beside the tree's, sharing its shape, including the read-and-decode interface, because a driver that ignores the interface the tree has moved to has a short life |
| The honest alternative | Whether the in-tree driver already drives this part, tried before a line is written, and reported either way |

**The thing to try first is still the cheap thing.** Point this part's node at `st,vl53l5cx`,
build, and see what happens on the wire. It may work, it may fail at the first identification
read, and either outcome is a paragraph worth more than a page of reasoning. A chapter that wrote
a driver without trying the existing one would have skipped the step a reader would ask about
first.

## The hardware is here, and fitting it costs something

**The shield is on the bench.** X-NUCLEO-53L8A1, carrying the VL53L8CX, eight by eight
multizone, confirmed present in the bench inventory as corrected on Wednesday 30 September 2026.
Nothing in this project is blocked on a part arriving.

Two costs come with fitting it, and both belong in the design rather than in a surprise later.

**The shields stack one at a time, and this one displaces the leads P04 used.** The three
X-NUCLEO shields are Arduino form factor and only one can be on the board at once. The marker
lead on CN7 pin 19 and the ground lead to the acquisition board come off when this shield goes
on. **Nothing is lost by that**, because P04's criterion 5 is recorded under both kernels, at
0.504 and 0.531 microseconds, and a recorded result is not re-taken to keep a wire warm. It does
mean the order is one way: P04's bench work is finished before this shield is fitted, and it is.

**The board is shared with the sibling firmware volume.** Fitting a shield is a bench action
that changes what that volume's projects see, so it is Joseph's to take and not a step this
design can assume. Every console step in this project is preceded by its own reflash for the
same reason P04's were.

**The bus selection on this shield is not settled on this bench, and there is no soldering
iron.** The chapter says so and this page does not improve on it by guessing. The arrangement is
to be read from the vendor's own schematic for this shield, not from a datasheet summary and not
from another maker's board: the lesson from Tuesday 6 October 2026 is that pad order and supply
arrangement vary between makers, and the schematic is what gives a value while the datasheet
gives it a meaning. **Two-wire first. The four-wire bus is described and not run**, which is the
chapter's own position.

## The bus is a description, which P03 already established

The two-wire controller is turned on by an overlay and not by a line in the application, and
that is not this chapter's discovery: [P03](../../03-one-sensor/docs/DESIGN.md) made the point
and proved it by building the same application object against two buses. This chapter inherits
the arrangement rather than repeating the argument, so its overlay enables `i2c1` with the same
pin groups from the board manual and RM0455, and hangs this part off it instead of an
accelerometer.

What is new here is that the node's `compatible` names a binding **this repository writes**, and
a binding is a description of hardware rather than a configuration file. The reset pin and the
data-ready line go in it, because they are facts about the wiring, and nothing in the
application names a pin.

## The seven criteria, sorted by what each actually needs

This is the part worth settling before any code, because it decides what can be finished when.
The chapter lists seven and they do not cost the same.

| | Criterion | Needs |
|---|---|---|
| 1 | the device comes up, and failure is reported as failure | the board, the shield, and one run with the shield deliberately absent |
| 2 | nothing and far are different facts | the board, the shield, and two scenes: an empty room and a wall at about two metres |
| 3 | one fetch serves sixty-four zones | the board, the shield, and a count of bus transactions |
| 4 | the application never polls | the board, the shield, and the data-ready line connected |
| 5 | the grid is the right way up | the board, the shield, and something opaque over one quadrant |
| 6 | P01 is unchanged | **a host only** |
| 7 | the licence travels with the code | **a host only** |

**Criteria 6 and 7 need no hardware and they are not the leftovers.** Criterion 6 is the whole
product argument of the chapter reduced to a comparison a machine can make: if P01's application
object is byte for byte what it was and only its range source differs, the claim that this part
is interchangeable with any other is proven rather than asserted. Criterion 7 is a rule about
the tree, so it can be enforced from the day the vendored source lands, and it is the one most
likely to be broken quietly by somebody updating a pin in a hurry.

So the order of work is: the binding and the wrapper, then criteria 6 and 7 with their checks in
continuous integration, then the shield, then the bench. That is not the order the chapter lists
them in, and the chapter is not wrong: it lists them by what they mean and this table sorts them
by what they cost.

**Criterion 1 has a shape this volume has met twice and should name once more.** An absent sensor
that reports zeroes looks exactly like an empty room, and a grid of zeroes is more convincing
than a single zero because it has structure. The guard is the same one P03 used for device
readiness and P04 used for a count that came back zero: a value that might not be a measurement
is not printed as one. Here that means the status array decides, not the distance array, which is
also criterion 2.

## The decision that goes beyond the chapter, and what it costs

**The driver is C and there is no choice in it.** A Zephyr driver is C, the vendor's source is C,
and a chapter that wrote the driver in anything else would be inventing a problem to show off a
solution. So the variation this volume carries across languages lands elsewhere in this project,
and the placement is a decision rather than an omission:

| Piece | Language | Why there |
|---|---|---|
| the binding and the wrapper | C, C11 | what the kernel takes; no alternative exists |
| the application that consumes the grid | C, and a second one in C++23 | the eight by eight geometry and the zone ordering are constants, so the C++ one checks them at compile time rather than in a test, which is the trick [P01](../../01-presence/) used for its guard order |
| the host-side reducer that turns a captured grid into an occupancy statement | Rust | the one piece that is neither a driver nor an application, so a third language costs nothing here and proves something: the same grid, reduced twice, with the device's own capture as the oracle |

**What it costs:** three descriptions of one grid, which is three chances to describe three
different grids, and no test inside any of them can notice, because each is right about itself.
That is the risk [P01](../../01-presence/) and [P02](../../02-the-claim/) were built to manage
and the answer is theirs: one shared test, fed from the device's own capture, rather than each
language passing a test of its own. **The zone ordering is written down once, before any of the
three exists**, and criterion 5 is what checks the written order against the physical one.

**What it does not license:** any claim that one language reads the grid faster. The figures in
this volume for anything at the scale of a bus transaction come from P04's method and P04's
instrument, and a comparison between an application in C and one in C++23 over the same driver
would be comparing two compilers' inlining decisions.

## The trap this project is most likely to produce

**A grid whose orientation is assumed.** Sixty-four numbers arrive in an order, that order maps
to a physical arrangement, and nothing in the data says which way up it is. Every consumer
downstream then inherits the assumption, and the first thing that notices is a product decision
made about the wrong corner of a room. Criterion 5 exists for exactly this and it is the cheapest
criterion to run and the easiest to skip: cover one quadrant, read which part of the printed grid
darkens, and write the answer down before anything downstream uses it.

The second most likely is **a fetch per zone**. The vendor's interface will offer a way to read
the grid, and a wrapper that loops over sixty-four zones calling it will work, be slow, and look
correct. Criterion 3 counts bus transactions rather than trusting the shape of the code.

## What this design does not claim

**Nothing about the ranging algorithm.** It is the vendor's, the source is used unmodified, and
its licence text is committed beside it. This chapter writes the binding, the wrapper and the
module arrangement.

**No person identification, and the volume never calls this part radar.** It times light,
resolves sixty-four zones, and produces nothing from which a person could be identified. The
literature on sensing people without cameras is largely about millimetre-wave radar and is cited
for its framing rather than its method, because the method does not transfer.

**No four-wire run.** Described, not built, for the reason above.

**No number at all, yet.** Every row of this project's eventual table reads `not measured` until
a run produces it. A protocol published as a protocol is honest; a protocol published as a result
is not.

## The parts, which are all on the bench

| Part | Role |
|---|---|
| NUCLEO-H7A3ZI-Q | the host for the driver, with its two-wire controller enabled by an overlay |
| X-NUCLEO-53L8A1, carrying the VL53L8CX | the subject. Eight by eight zones over the two-wire bus |
| one general-purpose pin | the sensor's reset, named in the binding |
| one interrupt line | data ready, which is what criterion 4 turns into a thread that never polls |

No acquisition board and no second instrument. This project measures nothing at the scale where
the external witness could see it, and P04 is the chapter that explains why that sentence is a
decision rather than a shortcut.
