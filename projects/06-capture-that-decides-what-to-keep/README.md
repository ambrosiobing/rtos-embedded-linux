# P06. Capture that decides what to keep

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 06](../../chapters/06-capture-that-decides-what-to-keep.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q with one motion and environment shield, and a twin on the host |
| Board | NUCLEO-H7A3ZI-Q with the X-NUCLEO-IKS4A1, the only shield fitted |
| Peripherals | The two-wire bus and one interrupt line, as P03 established; no transfer engine and no signal processing accelerator |
| Toolchain | As the front matter, under both kernels exactly as P04 built them, plus a host twin in Python that produces the input and checks the output |
| Operating system | Three threads joined by two kernel objects, a message queue between sample and gate and a work queue between gate and record, under FreeRTOS and under Zephyr; fixed windows, no allocation after start |
| Difficulty | 4 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

The cost of a sixteen-byte message through each kernel's queue on this board, replicated against the published Thread Metric figure before the pipe is built on it; then a pipe with a stated bound on bytes per hour, a loss figure measured against the raw stream, and a test suite whose inputs include the ones the pipe has to refuse

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
