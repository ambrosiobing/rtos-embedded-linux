# P11. Cellular from the RTOS: attach, and one payload

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 11](../../chapters/11-cellular-from-the-rtos.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q driving a SIM7070G module, with a broker on a Raspberry Pi |
| Board | NUCLEO-H7A3ZI-Q, a SIM7070G expansion board on its own supply, a Raspberry Pi 4 running the broker |
| Peripherals | One serial port to the module; no flow control, deliberately, and the cost of that is counted |
| Toolchain | As the front matter; the module's own command set read from its documentation rather than from the driver |
| Operating system | The modem subsystem, a point-to-point link layer, sockets, and the message client |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

A device that attaches to a network by itself, publishes one record over an authenticated connection, and logs every counter that would let somebody at a distance tell why it did not

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
