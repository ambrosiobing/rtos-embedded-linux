# P02. The claim, and the service axis

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 02](../../chapters/02-the-claim.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, and nothing else |
| Board | NUCLEO-H7A3ZI-Q alone. The user button stands in for a panel button |
| Peripherals | The three indicators, now driven by the claim rather than by presence; the virtual console; two kernel timers, for the grace period and for the longer hold |
| Toolchain | As P01, plus the scripted-event runner |
| Operating system | One thread, the same queue discipline as P01 |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

One policy function with seven reason codes, a service axis that runs beside the booking state, and a spool that keeps working when the network does not

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
