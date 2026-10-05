# P15. Twister on hardware, on every push

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 15](../../chapters/15-twister-on-hardware.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | The NUCLEO-H7A3ZI-Q as a device under test on a Raspberry Pi 4 runner |
| Boards | A NUCLEO-H7A3ZI-Q as the device under test, attached to a Raspberry Pi 4 that is a self-hosted runner |
| Peripherals | The debug probe for flashing and the virtual console for reading; nothing else |
| Toolchain | As the front matter, plus the framework's own test runner and a hardware map |
| Operating system | The assertion library on the target; the runner is Linux |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

A hardware map, tests from six chapters running on the board on every push, and a suite that fails when a chapter's claim stops being true

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
