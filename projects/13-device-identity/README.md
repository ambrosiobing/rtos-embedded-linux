# P13. Device identity: enrol, rotate, revoke

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 13](../../chapters/13-device-identity.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | A Raspberry Pi 4 as the authority, a Pi 3B+ as a Linux unit, and the Cortex-M7 board as a microcontroller unit |
| Boards | A Raspberry Pi 4 holding the authority, a Pi 3B+ as a Linux unit, and the NUCLEO-H7A3ZI-Q as a microcontroller unit |
| Peripherals | The storage partition P07 placed above both image slots; the serial port of P09 for the enrolment of the microcontroller unit |
| Toolchain | As the front matter, plus the authority's own tooling on the Pi 4 |
| Operating system | Both of them: this is the one chapter where the same lifecycle runs on a microcontroller and on Linux |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

One process by which a unit of either kind acquires an identity, renews it before it expires, and can be refused afterwards, with the refusal demonstrated rather than asserted

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
