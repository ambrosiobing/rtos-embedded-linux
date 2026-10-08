# P04. What the kernel primitives cost, by the cycle counter

Status on Thursday 8 October 2026: **the design page exists and no code does.** No number
here has been measured and every row of the eventual table reads `not measured`.

[docs/DESIGN.md](docs/DESIGN.md) sorts the chapter's seven acceptance criteria by what each
costs, and **two of them need no hardware at all**: that the reduction can report a
disagreement it was shown, and that no row of the table cites an instrument which cannot see
what the row claims. Those two come first, because a check that cannot fail has shown nothing.

It also records the one decision that goes beyond the chapter, with its cost: the same
primitives will be priced under **two kernels**, so that a ratio between primitives can be
shown to survive a change of kernel rather than being a property of one.

The written specification is [chapter 04](../../chapters/04-what-the-kernel-primitives-cost.md),
which is complete. What is absent is the code.

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

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
