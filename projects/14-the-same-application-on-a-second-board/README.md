# P14. The same application on a second board

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 14](../../chapters/14-the-same-application-on-a-second-board.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | The STEVAL-STWINBX1, a Cortex-M33 industrial sensor node |
| Board | STEVAL-STWINBX1, a Cortex-M33 with its own sensors, its own radio and its own flash layout. A USB device, not a shield |
| Peripherals | The board's own motion sensors, reached through the same interface P03 established; its radio through a serial host-controller binding |
| Toolchain | As the front matter; flashing by the board's own bootloader or by an external probe, and the chapter names which it used |
| Operating system | The same, built for a second processor family |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

P06's application running on a board it was not written for, with every file that had to change listed and justified

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
