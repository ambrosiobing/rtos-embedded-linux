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
| Toolchain | As the front matter, plus the upstream management client on the gateway, since the protocol has several and this chapter's work is on the unit |
| Operating system | The device application of P08 with the management server on its own work queue, receiving chunks and writing a slot while P01 keeps running; a broker, an image store and the upstream client on the gateway |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

An image that travels from a store to a unit over a serial port, with the upload and the swap timed against image size and the slopes predicted first, and P01's period held through the upload; a published state carrying a sequence number and a staleness flag; and a shadow on which P02's cases run against a fleet rather than against one board

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
