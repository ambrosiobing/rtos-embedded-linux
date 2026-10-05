# P09. Fleet update and the fleet shadow

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 09](../../chapters/09-fleet-update-and-the-fleet-shadow.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q over its serial port to a Raspberry Pi gateway |
| Board | NUCLEO-H7A3ZI-Q, and a Raspberry Pi 4 as the gateway |
| Peripherals | One serial port between them. There is no Ethernet on the microcontroller board, and that is the premise of the chapter rather than an obstacle to it |
| Toolchain | As the front matter, plus a host client in C written here, because none exists upstream |
| Operating system | The device application of P08; a broker, an image store and the client on the gateway |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

An image that travels from a store to a unit over a serial port, a published state carrying a sequence number and a staleness flag, and a shadow on which P02's cases run against a fleet rather than against one board

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
