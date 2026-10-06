# P05. The zones: an out-of-tree driver

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 05](../../chapters/05-the-zones.md), which is
complete. What is absent is the code.

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
