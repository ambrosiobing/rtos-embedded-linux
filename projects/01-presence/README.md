# P01. Presence: free, occupied, held, fault

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 01](../../chapters/01-presence.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, and nothing else |
| Board | NUCLEO-H7A3ZI-Q alone. No shield, no instrument, no radio |
| Peripherals | The three on-board indicators as a state display, the virtual console for the event log, the user button as a forced event, one kernel timer |
| Toolchain | The workspace from the front matter, pinned; west and CMake |
| Operating system | The real-time operating system, one thread and one work queue |
| Difficulty | 3 of 5 |
| Effort | 3 evenings of about four hours |

## What it would deliver

A transition table whose every row is proven reachable by a test that runs on the host with no board attached, and a release that cannot be lost

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
