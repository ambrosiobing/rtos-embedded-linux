# Twenty Connected-Device Projects on an RTOS and Embedded Linux

*a room that knows whether it is in use*

Joseph Ambrose Pagaran. Friday 2 October 2026.

Twenty projects that together make one thing: a room that knows whether it is in
use, says so to whoever asks, accepts a new version of itself without being
visited, and runs on a battery while doing it. Fifteen run a real-time operating
system on a microcontroller, three run Linux on a single-board computer, and two
sit on the boundary. Everything is built on hardware already on the bench.

## The four behaviours everything else serves

| Behaviour | Built in | Why it is the one that matters |
|---|---|---|
| Presence, with a hold and a release | [P01](chapters/01-presence.md) | A room that forgets to release is worse than a room with no sensor |
| A sample that is allowed to leave the device | [P06](chapters/06-capture-that-decides-what-to-keep.md) | Everything that costs energy or privacy is decided before a radio is involved |
| A signed image that rolls back | [P08](chapters/08-a-signed-image.md) | A unit that cannot be reached by hand needs an update that can fail safely |
| A radio that sleeps | [P10](chapters/10-a-radio-that-sleeps.md) | A battery device is a duty cycle with a radio attached |

A reader with time for four chapters should read those four, in that order.

The sentence the volume is built to earn: the unit does not own the calendar. It
owns the decision when the calendar and the body disagree, and it has to make
that decision with the radio down.

## The twenty projects

| NN | Title | Target | Idiom it proves |
|----|-------|--------|-----------------|
| [01](projects/01-presence) | [Presence: free, occupied, held, fault](chapters/01-presence.md) | Nucleo alone | maintainability |
| [02](projects/02-the-claim) | [The claim, and the service axis](chapters/02-the-claim.md) | Nucleo alone | validation |
| [03](projects/03-one-sensor) | [One sensor, two buses, zero code changes](chapters/03-one-sensor.md) | Nucleo + ADXL345 | maintainability |
| [04](projects/04-what-the-kernel-primitives-cost) | [What the kernel primitives cost, by the cycle counter](chapters/04-what-the-kernel-primitives-cost.md) | Nucleo + MCC 118 on a Pi | performance |
| [05](projects/05-the-zones) | [The zones: an out-of-tree driver](chapters/05-the-zones.md) | Nucleo + 53L8A1 | maintainability |
| [06](projects/06-capture-that-decides-what-to-keep) | [Capture that decides what to keep](chapters/06-capture-that-decides-what-to-keep.md) | Nucleo + IKS4A1 | efficiency |
| [07](projects/07-settings-that-survive-a-power-cut) | [Settings that survive a power cut](chapters/07-settings-that-survive-a-power-cut.md) | Nucleo + PPK2 | reliability |
| [08](projects/08-a-signed-image) | [A signed image, confirm and revert](chapters/08-a-signed-image.md) | Nucleo, sysbuild | reliability |
| [09](projects/09-fleet-update-and-the-fleet-shadow) | [Fleet update and the fleet shadow](chapters/09-fleet-update-and-the-fleet-shadow.md) | Nucleo + Pi 4 gateway | observability |
| [10](projects/10-a-radio-that-sleeps) | [A radio that sleeps](chapters/10-a-radio-that-sleeps.md) | Nucleo + PPK2 + SIM7070G | sustainability |
| [11](projects/11-cellular-from-the-rtos) | [Cellular from the RTOS: attach, and one payload](chapters/11-cellular-from-the-rtos.md) | Nucleo + SIM7070G | observability |
| [12](projects/12-wi-fi-on-a-second-architecture) | [Wi-Fi on a second architecture](chapters/12-wi-fi-on-a-second-architecture.md) | ESP32 + Pi 4 broker | portability |
| [13](projects/13-device-identity) | [Device identity: enrol, rotate, revoke](chapters/13-device-identity.md) | Pi 4, Pi 3B+, Nucleo | reliability |
| [14](projects/14-the-same-application-on-a-second-board) | [The same application on a second board](chapters/14-the-same-application-on-a-second-board.md) | STWIN.box | portability |
| [15](projects/15-twister-on-hardware) | [Twister on hardware, on every push](chapters/15-twister-on-hardware.md) | Nucleo on a Pi 4 runner | validation |
| [16](projects/16-the-extensible-sdk) | [The extensible SDK, devtool, and a CI that builds with it](chapters/16-the-extensible-sdk.md) | Pi 4 | maintainability |
| [17](projects/17-first-boot) | [First boot, factory reset, decommission](chapters/17-first-boot.md) | Pi 3B+ + Explorer700 | reliability |
| [18](projects/18-canopen-on-a-real-wire) | [CANopen on a real wire, and on no wire at all](chapters/18-canopen-on-a-real-wire.md) | Nucleo + transceiver + Pi 4 | validation |
| [19](projects/19-a-tunnel-as-the-management-plane) | [A tunnel as the management plane](chapters/19-a-tunnel-as-the-management-plane.md) | Pi 4, NanoPi, Pi 3 + SIM7600E-H | validation |
| [20](projects/20-a-bridge-between-two-message-protocols) | [A bridge between two message protocols](chapters/20-a-bridge-between-two-message-protocols.md) | Pi 4, two brokers | validation |

The last three open by saying that they exist because a requirement list named
them rather than because the device needs them, and each then finds the one
place it genuinely contributes. A reader short of time can skip all three
without losing the argument.

## Four threads

- **What the device decides.** [P01](chapters/01-presence.md), [P02](chapters/02-the-claim.md), [P05](chapters/05-the-zones.md), [P06](chapters/06-capture-that-decides-what-to-keep.md).
- **What it accepts from outside.** [P07](chapters/07-settings-that-survive-a-power-cut.md), [P08](chapters/08-a-signed-image.md), [P09](chapters/09-fleet-update-and-the-fleet-shadow.md), [P13](chapters/13-device-identity.md).
- **What it costs.** [P04](chapters/04-what-the-kernel-primitives-cost.md), [P10](chapters/10-a-radio-that-sleeps.md), and [P06](chapters/06-capture-that-decides-what-to-keep.md) where the two meet.
- **Whether it survives being changed.** [P03](chapters/03-one-sensor.md), [P12](chapters/12-wi-fi-on-a-second-architecture.md), [P14](chapters/14-the-same-application-on-a-second-board.md), [P15](chapters/15-twister-on-hardware.md), [P16](chapters/16-the-extensible-sdk.md).

## Honesty rules this volume keeps

- Every measurement names its instrument, or reads `not measured`. On Friday 2
  October 2026 most of them read `not measured`, and the appendix says so in one
  place rather than making a reader count.
- Solid outlines are hardware on the bench, dashed is a model standing in,
  dotted is hardware that is absent with the cost of its absence written down.
  No measurement drawn through a dashed or dotted block is a physical one.
- No chapter claims a shipped product on this operating system, a battery life
  in years, or radar. Time of flight is time of flight.
- Eleven things are not built at all, and the appendix gives the reason for each.
  Six of those are refusals to claim something.
- Twelve things a reader might look for here are built in a sibling volume and
  are cited rather than described twice.

## There is no code here yet, and the READMEs say so

This volume is twenty written chapters. **No project has been built**, and each
of the twenty `projects/NN-slug/README.md` files says that in its first line,
names what it would need on the bench, and links to the chapter that designs it.

The directories exist anyway, all twenty from the start, so the shape of the
volume is visible in the repository rather than only in the book. A missing
directory would let a reader assume work was coming; a README that implied work
had started would be a claim with nothing behind it. Every figure in this volume
was rendered and looked at, and that is the only thing here that has been done
rather than designed.

## Building

**Nothing needs building to read this.** Everything published here is Markdown
with its figures beside it as SVG, so a clone needs no interpreter, no TeX
installation and no build step.

What is published is generated from LaTeX, and **the LaTeX is not published**. It
stays on the authoring machine, which is why a clone has no `sections/` and no
`main.tex`. Two commands turn one into the other:

    python mdbuild.py --out build/md     the chapters, converted, into scratch
    python scripts/publish.py            scratch into the published shape

`scripts/publish.py` is the only thing that writes `chapters/`. It puts each
chapter's five figures with the project they belong to, rewrites every image link
to match, and normalises the typography a typeset page wants and a Markdown file
does not, which is the thin space `\,` produces between digit groups and the
curly quote a plural possessive produces. Both are invisible on the page and both
break a reader's search.

`scripts/make_projects.py` writes the twenty project READMEs, reading every field
out of the chapter's own key facts so a stub cannot disagree with its chapter. It
never overwrites a README that already exists, because the day a project starts
its README stops being a stub.

**There is no LaTeX here, figure sources included.** This repository holds one
markup language: Markdown, with the figures as SVG because Markdown draws no
diagram in a browser without them. The TikZ that draws each figure is part of the
authoring half and stays with the chapter sources. The workflow refuses a push
that commits a `.tex`, or any file type other than Markdown, SVG and the scripts
that produce them.

The PDF and the single-file HTML are for the author's own proofreading and are
**never committed and never published**, in whole or per chapter. A reader takes
one chapter and one project, not an assembled volume. `.gitignore` enforces that,
and so does the `checks` workflow.

## Checks

These run on a clone, with nothing but a Python interpreter:

    python scripts/lint.py               house rules, and the name scan
    python scripts/check_links.py        every relative link and anchor

`scripts/lint.py` checks prose for em and en dashes, non-ASCII characters and
violent idioms, and code blocks for non-ASCII and over-long lines. It also runs
the scan that keeps this volume generic: it was written with one advertisement
open and prints no employer, product, town or region, and each excluded token is
stored as a hash so the rule is enforced without the list ever being written
down here. A table rule, a horizontal rule and an inline code span are stripped
before prose is examined, because a double dash inside a command is correct.

`scripts/check_links.py` is what proves the figures are where the chapters say
they are. The linter reads words and the name scan reads words; neither looks at
a link, and a wrong rewrite would be a page of broken images.

Two more checks are structural and live in
[`.github/workflows/checks.yml`](.github/workflows/checks.yml): twenty chapters
with twenty projects and five figures each as both SVG and TikZ, and no LaTeX
committed outside a figure directory.

The authoring machine has three more that a clone does not need, because they
read the LaTeX: `python lint.py`, `python crosscheck.py`, and
`sh figcheck.sh z05_arch` to render one figure to PNG and look at it.

## Repository layout

| Path | What it is |
|---|---|
| `chapters/NN-slug.md` | the volume, one file per chapter, 00 to 21. Written only by `scripts/publish.py` |
| `projects/NN-slug/README.md` | that project's state, which today is not started |
| `projects/NN-slug/docs/figures/` | that chapter's five figures, as SVG |
| `docs/figures/front_stack.svg` | the one figure belonging to no single project |
| `scripts/` | `publish.py`, `make_projects.py`, `lint.py`, `check_links.py` |

Every file in this repository is Markdown, SVG, one of those four Python scripts,
the workflow, the licence or the citation file. Nothing else, and the workflow
checks it.

`AUTHORING.md` is the contract every chapter follows. `SOURCE.md` is the
prior-art pool, with a licence and a reading date on every row, and with the
reason each of its three cited papers is not the design.

**What is deliberately not here.** `sections/*.tex`, `main.tex`,
`tikz_preamble.tex`, `figures/*.tex`, `build.py`, `mdbuild.py`, `lint.py`,
`crosscheck.py` and `figcheck.sh` are the authoring half. They stay on the
authoring machine and `.gitignore` keeps them out, so a clone carries one copy of
each chapter rather than two in two markup languages. The workflow refuses a push
that puts any of them back.

Figures as SVG are the one deliberate exception to not committing built output,
because the Markdown draws nothing in a browser without them.
