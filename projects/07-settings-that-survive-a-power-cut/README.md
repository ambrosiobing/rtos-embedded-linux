# P07. Settings that survive a power cut

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 07](../../chapters/07-settings-that-survive-a-power-cut.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q with the power meter as a switched supply |
| Board | NUCLEO-H7A3ZI-Q, powered through the meter so that the supply can be cut on command |
| Peripherals | The internal flash, through the storage partition this chapter defines; the virtual console; one general-purpose pin to mark a write for the meter's digital input |
| Toolchain | As the front matter, plus a host script that cuts the supply at scripted moments and counts what survived |
| Operating system | One thread, the settings subsystem over non-volatile storage |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

The volume's flash layout, a versioned configuration that migrates forward, and a thousand power cuts with the survival rate reported rather than assumed

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
