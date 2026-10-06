# P16. The extensible SDK, devtool, and a CI that builds with it

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 16](../../chapters/16-the-extensible-sdk.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | A Raspberry Pi 4 as the target, and a build host that never compiles the same thing twice |
| Board | A Raspberry Pi 4 as the target. The build itself runs on a host with more memory and more patience |
| Peripherals | None. This chapter is about a build |
| Toolchain | The build system from the sibling Linux volume, plus its extensible software development kit and the development tool that goes with it |
| Operating system | Linux on both sides |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

A recipe for the gateway software of P09, an extensible kit that a developer can use without the whole build system, and a continuous integration job that cross-compiles with a cached kit and deploys

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
