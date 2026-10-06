# P20. A bridge between two message protocols

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 20](../../chapters/20-a-bridge-between-two-message-protocols.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | A Raspberry Pi 4 running both brokers, with the bridge in C between them |
| Board | A Raspberry Pi 4 running both brokers and the bridge. No device, no radio, no instrument |
| Peripherals | None |
| Toolchain | As the front matter, plus the second broker's C client library |
| Operating system | Linux |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

A bridge that forwards every record from one broker to the other at least once, loses none across a restart on either side, and is proven by a replay that counts rather than by a claim

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
