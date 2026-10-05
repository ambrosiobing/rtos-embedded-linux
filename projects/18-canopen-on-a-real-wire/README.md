# P18. CANopen on a real wire, and on no wire at all

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 18](../../chapters/18-canopen-on-a-real-wire.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | The NUCLEO-H7A3ZI-Q through a transceiver to an expansion board on a Raspberry Pi 4 |
| Boards | NUCLEO-H7A3ZI-Q as the node, a Raspberry Pi 4 as the master. A transceiver board between the Nucleo and the wire |
| Peripherals | The controller on the processor, in classic mode; an expansion board on the Pi with its own controller and transceiver |
| Toolchain | As the front matter, plus the profile implementation as a module and a host-side stack on the Pi |
| Operating system | The same, on the node; the simulated target on the continuous integration runner |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

A node with an object dictionary that a master can read, a heartbeat whose absence means something specific, and the same application running with no wire at all on the runner

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
