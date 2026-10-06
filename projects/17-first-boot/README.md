# P17. First boot, factory reset, decommission

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 17](../../chapters/17-first-boot.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | A Raspberry Pi 3B+ with the Explorer expansion board, whose joystick is the held button |
| Board | A Raspberry Pi 3B+ with the JOY-iT Explorer expansion board, whose joystick is the only physical control in this volume |
| Peripherals | The joystick as the held button; one indicator for what the hold is doing; the data partition where a credential lives |
| Toolchain | As the front matter, plus the layer of P16 for packaging |
| Operating system | Linux |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

A unit that configures itself on first boot, that can be returned to a known state by somebody holding a control for ten seconds, and that can be removed from service with its credential provably gone

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
