# P04. What the kernel primitives cost, by the cycle counter

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 04](../../chapters/04-what-the-kernel-primitives-cost.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, with an MCC 118 on a Raspberry Pi as a witness for periods only |
| Board | NUCLEO-H7A3ZI-Q, with a Raspberry Pi carrying an MCC 118 as an external witness |
| Peripherals | The data watchpoint unit's cycle counter, one general-purpose pin for the witness, the virtual console |
| Toolchain | As the front matter, plus a short host script that reduces the captured periods |
| Operating system | Several threads, a message queue, a work queue, a mutex |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

A table of primitive costs in cycles, each with its own method and its own refutation, and a witness that confirms the one claim the cycle counter cannot make about itself

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
