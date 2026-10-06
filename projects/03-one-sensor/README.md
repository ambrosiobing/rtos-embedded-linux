# P03. One sensor, two buses, zero code changes

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 03](../../chapters/03-one-sensor.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q with an ADXL345 breakout, on jumper leads |
| Board | NUCLEO-H7A3ZI-Q with a DFRobot SEN0032 carrying an ADXL345, on jumper leads |
| Peripherals | One two-wire bus and one four-wire bus, each enabled by overlay because the board's own description enables neither; one interrupt line for the data-ready trigger |
| Toolchain | As the front matter, plus the devicetree compiler's own output for reading |
| Operating system | One thread, polling in the first build and waiting on a trigger in the second |
| Difficulty | 3 of 5 |
| Effort | 3 evenings of about four hours |

## What it would deliver

One application binary built four ways from identical C, differing only in which overlay is given to the build

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
