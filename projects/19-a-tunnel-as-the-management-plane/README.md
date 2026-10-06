# P19. A tunnel as the management plane

Status: not started. Nothing in this directory has been built, and no
number here has been measured.

The written design is [chapter 19](../../chapters/19-a-tunnel-as-the-management-plane.md), which is
complete. What is absent is the code.

| | |
|---|---|
| Target | A Raspberry Pi 4 as the server, a NanoPi NEO Air over wireless, and a Raspberry Pi 3 behind a carrier's network |
| Boards | A Raspberry Pi 4 as the server; a NanoPi NEO Air joining over the building's wireless network; a Raspberry Pi 3 with a cellular expansion board, behind a carrier's network |
| Peripherals | One cellular expansion board on the Pi 3; the NanoPi takes no expansion board, because its header is not the forty-pin kind |
| Toolchain | As the front matter, plus the layer of P16, which packages the daemon |
| Operating system | Linux throughout. No microcontroller unit is involved |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What it would deliver

A management plane that a unit behind a carrier's network can reach, that nothing on the building network can, and that refuses a device revoked in P13

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
