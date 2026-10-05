# P08. A signed image, confirm and revert

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 08](../../chapters/08-a-signed-image.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, built with the bootloader alongside the application |
| Board | NUCLEO-H7A3ZI-Q, using the partition layout P07 settled |
| Peripherals | The internal flash through the two image slots; the independent watchdog; the virtual console |
| Toolchain | As the front matter, plus the build-system wrapper that produces the bootloader and the application together, and the image signing tool |
| Operating system | The application unchanged from P01; the bootloader runs before it |
| Difficulty | 5 of 5 |
| Effort | 5 evenings of about four hours |

## What it would deliver

A signed image that boots, a test image that must confirm or be replaced, a watchdog that enforces the deadline, and a demonstration that an image which never confirms is not the one running afterwards

## Figures

The chapter's five figures are in [docs/figures](docs/figures), each as
the rendered SVG and the TikZ source that draws it.
