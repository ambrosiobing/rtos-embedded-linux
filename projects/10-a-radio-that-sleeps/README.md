# P10. A radio that sleeps

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 10](../../chapters/10-a-radio-that-sleeps.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q with the power meter in series, and a cellular module on its own supply |
| Board | NUCLEO-H7A3ZI-Q, supplied through the meter; a SIM7070G module powered from its own connector |
| Peripherals | The device power management subsystem, the idle thread, one serial port to the module, one pin marking each phase for the meter |
| Toolchain | As the front matter, plus the meter's host library and a short reduction script |
| Operating system | One thread per phase, device runtime management on the peripherals that support it |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

One table of current per state, measured, with each row naming what was running; and the two link timers logged as granted rather than as requested

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
