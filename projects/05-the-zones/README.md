# P05. The zones: an out-of-tree driver

Status on Saturday 17 October 2026: **the design page exists and no code does.** Nothing in
this directory has been built and no number here has been measured.

[docs/DESIGN.md](docs/DESIGN.md) sorts the chapter's seven acceptance criteria by what each
costs, and **two of them need no hardware at all**: that P01's application object is byte for
byte what it was with only its range source changed, and that the vendor's licence travels with
the vendored source at a pinned commit. Those two come first.

**That check has run, and it reshaped the project.** The tree carries no driver for this part and
a complete one for the generation before it, nine files under `drivers/sensor/st/vl53l5cx/`,
written against the read-and-decode interface the tree has moved to. So this is not a family the
tree has never met: the work is to bring the next generation into a tree that already drives the
previous one, which is the harder job and the one an engineer actually meets. The chapter's
instinct to vendor a parallel vendor driver is withdrawn on the page rather than quietly dropped.

The cheap thing is still tried first: point this part's node at the existing compatible string,
build, and see what happens on the wire. A chapter that wrote a driver without trying the
existing one would have skipped the step a reader asks about first.

The shield is on the bench. Fitting it takes the other leads off the board, which costs nothing
now that P04's bench work is finished, and it is a bench action rather than something this page
can assume.

The written specification is [chapter 05](../../chapters/05-the-zones.md), which is complete.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q with the eight by eight ranging shield, two-wire bus first |
| Board | NUCLEO-H7A3ZI-Q with the X-NUCLEO-53L8A1, the only shield fitted |
| Peripherals | The two-wire bus, enabled by the overlay of P03; one interrupt line for data ready; one general-purpose pin to reset the sensor |
| Toolchain | As the front matter, plus a west manifest entry that pulls the vendor driver in as a module |
| Operating system | One thread waiting on the trigger, no polling |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

A binding, a driver and a module that let the sixty-four zone sensor be used through the same interface as a three-axis accelerometer, with the vendor's code vendored rather than rewritten

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
