# P12. Wi-Fi on a second architecture

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 12](../../chapters/12-wi-fi-on-a-second-architecture.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | An ESP32 module on its own connector, with the broker of P09 on a Raspberry Pi |
| Board | An ESP32 module on its own USB connector. No wire to the Cortex-M7 board anywhere in this chapter |
| Peripherals | The module's own wireless radio; no shield, no sensor, no instrument |
| Toolchain | As the front matter, plus one command to fetch the binary hardware support package this target needs |
| Operating system | The same operating system, built for a different instruction set |
| Difficulty | 3 of 5 |
| Effort | 3 evenings of about four hours |

## What it would deliver

The message client of P11, unmodified, running on a second architecture, with a wireless join that reports its failures as specifically as P11's attach reports its own

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
