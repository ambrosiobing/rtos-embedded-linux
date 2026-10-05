# Authoring guide for the connected-devices volume

Read `sections/z01.tex` and `figures/z01_*.tex` first: they are the reference for
tone, depth, structure and figure style. Every chapter must compile alone with

    python build.py --check sections/zNN.tex

and finish with `== RESULT: CLEAN` (no pdflatex errors, all five figures render
to SVG, no unknown macro, no non-ASCII character in a code block, no code line
over its limit). Then

    python lint.py sections/zNN.tex

must print `clean`, and

    python crosscheck.py

must report no cross-chapter problems.

A label must never sit on top of a symbol or another label. Circuit labels go
beside the branch, not centred on the component; sequence-diagram notes go
outside the lifelines; memory-map addresses go outside the column. Render the
figure and look at it before moving on. The linter cannot see this and three
overlapping figures reached a sibling volume before anyone did.

Before a release, prove the toolchain has not drifted from its sibling:

    python build.py --drift ../EmbeddedLinux_KitLabs20/build.py

Everything outside the `DOC` block should match.

---

## THE RULES THAT HAVE NO EXCEPTIONS

**1. No employer, no product, no town, no region, no portal.** This volume was
written with one advertisement open and must serve any application. A named
company dates the work to one vacancy and makes the book useless the week that
vacancy closes. The device in these pages is a **room** or a **unit**. It is
never the product noun. `lint.py` enforces this, and it does so by comparing
hashes rather than by listing the words, because a rule that spells what it
forbids puts the forbidden thing into the tree it protects. The plaintext list
lives once, in the private catalogue note, and never here.

**2. Nothing anywhere about how this was produced, or about the tools that
helped produce it.** Not in prose, not in a comment, not in a commit message,
not in a trailer, and not in a directory that configures such a tool. This rule
deliberately does not name them. The scan that enforces it lives outside this
repository on purpose and is run by hand before anything is published.

**3. No chapter claims a shipped product on this operating system, a
three-year battery, or radar.** Time of flight is time of flight. The volume may
say what a technique is for; it may not say that the author shipped it where he
did not. Where a chapter is a prototype, the first paragraph says so.

**4. Every measurement names its instrument, or reads "not measured".** There
are no invented microamps and no borrowed cycle counts. A budget table whose
Measured column is empty is a defect; a budget table whose Measured column reads
`not measured` is honest and passes.

**5. Three outline styles, used consistently in every architecture figure.**
Solid for hardware present on the bench. Dashed for a model or a synthetic
source standing in for hardware. Dotted for hardware that is absent, with one
sentence saying what its absence costs. No measurement drawn through a dashed or
dotted block is a physical measurement.

**6. Dates are written in full and redundantly:** weekday, day, month, year, on
every mention. `crosscheck.py` enforces it. The one exemption is `\pubdate{}`,
for a date a publisher gave to month precision only.

**7. No em dashes, no en dashes, no `--` in prose. No violent idioms.**

---

## What this volume does not build

Naming these once here is cheaper than four chapters discovering them
separately.

- **A ticketing client on the device.** The device's job ends at a state, a
  reason code and an identifier. A person decides to send an engineer.
- **A prediction of service life.** A current measurement is a current
  measurement. The arithmetic from there to a number of years needs a duty cycle
  nobody on this bench has observed for a year.
- **An automatic order for a spare part.** A rising number is a reason code.
- **A second A/B update story for the Linux side.** `../EmbeddedLinux_Top20`
  Project 19 owns it and is cited.
- **An oscilloscope trace, a logic-analyser capture, or a soldered joint.** None
  of the three exists on this bench. Every header is female.

---

## The product spine

Four behaviours carry the volume. Each is built in full by exactly one chapter,
and every other chapter that needs it cites that chapter and opens with the
`spine` box saying which behaviour it is serving. `crosscheck.py` reads this
table and fails if an owner chapter carries no spine box.

| Behaviour | Owner | Cited by |
|---|---|---|
| Presence with a hold and a release | P01 | P02, P05, P08, P09 |
| A sample that is allowed to leave the device | P06 | P05, P11, P12, P14 |
| A signed image that rolls back | P08 | P07, P09, P13 |
| A radio that sleeps | P10 | P04, P11 |

The one sentence the volume is built to earn: the unit does not own the
calendar. It owns the decision when the calendar and the body disagree, and it
must still make that decision with the radio down.

---

## The twenty projects

Numbering is fixed. Titles may be polished but not renamed in spirit.

| NN | Title | Target | Theme |
|----|-------|--------|-------|
| 01 | Presence: free, occupied, held, fault | Nucleo alone | The one state machine of the volume |
| 02 | The claim, and the service axis | Nucleo alone | Policy when the calendar and the body disagree |
| 03 | One sensor, two buses, zero code changes | Nucleo + ADXL345 | Devicetree, bindings, the sensor API |
| 04 | What the kernel primitives cost, by the cycle counter | Nucleo + MCC 118 on a Pi | Threads, queues, work queues, priority inheritance |
| 05 | The zones: an out-of-tree driver | Nucleo + 53L8A1 | A kernel driver with its own binding and module |
| 06 | Capture that decides what to keep | Nucleo + IKS4A1 | What may leave the device, and what it costs |
| 07 | Settings that survive a power cut | Nucleo + PPK2 | Flash partitions, NVS, a versioned schema |
| 08 | A signed image, confirm and revert | Nucleo, sysbuild | The bootloader, its signature and its rollback |
| 09 | Fleet update and the fleet shadow | Nucleo + Pi 4 gateway | Delivery over a wire that is not a network |
| 10 | A radio that sleeps | Nucleo + PPK2 + SIM7070G | One current table for the whole volume |
| 11 | Cellular from the RTOS: attach, and one payload | Nucleo + SIM7070G | The modem subsystem and the one message client |
| 12 | Wi-Fi on a second architecture | ESP32 + Pi 4 broker | The same client, a different silicon |
| 13 | Device identity: enrol, rotate, revoke | Pi 4, Pi 3B+, Nucleo | A certificate lifecycle across two operating systems |
| 14 | The same application on a second board | STWIN.box | Portability across two microcontroller families |
| 15 | Twister on hardware, on every push | Nucleo on a Pi 4 runner | The framework's own test runner against real hardware |
| 16 | The extensible SDK, devtool, and a CI that builds with it | Pi 4 | A build system's SDK, consumed outside it |
| 17 | First boot, factory reset, decommission | Pi 3B+ + Explorer700 | The first day and the last day of a unit |
| 18 | CANopen on a real wire, and on no wire at all | Nucleo + transceiver + Pi 4 HAT | A field protocol, and what a silent node means |
| 19 | A tunnel as the management plane | Pi 4, NanoPi, Pi 3 + SIM7600E-H | Reachability, and refusing a revoked device |
| 20 | A bridge between two message protocols | Pi 4, two brokers | At-least-once across a protocol boundary |

Cross-reference other chapters as `P05` in prose; `\ref{sec:z05}` works.

### Non-duplication with the four sibling volumes

- `../EmbeddedFirmware_NucleoH7_Top20` is the same board, bare metal. Chapter 20
  there builds this operating system's variant of the node **in full** and owns
  the interrupt-to-thread latency distribution. P04 here prices the kernel
  primitives against each other and prints no latency headline.
- Chapter 12 there owns the self-hosted runner on the Pi 4, the flashing step and
  the console fixtures. P15 here adds only the framework's test runner and says
  so in its first paragraph.
- Chapter 8 there is a bare-metal sampling schedule. P01 here is a room's
  occupancy with a hold, and its output is a lamp.
- `../EmbeddedLinux_Top20` Project 1 owns the image **and** the standard SDK.
  P16 here owns the extensible SDK and the CI that consumes it.
- Project 18 there issues certificates once for one broker. P13 here is the
  lifecycle: rotation, revocation, and two operating systems.
- Project 19 there owns the A/B update, the read-only root, the watchdog and the
  overlay on `/etc`. P17 here owns the first boot and the decommission only.
- `../EmbeddedRobotics_JointNode20` chapters 9 to 13 own an own protocol over
  this bus, its loopback bring-up and its network management. P18 here is the
  standard profile through the framework's own bus API.
- `../EmbeddedLinux_KitLabs20` lab 4 puts the STWIN.box on the bench running its
  vendor firmware. P14 here runs this operating system on it.
- The STWIN.box's condition-monitoring build belongs to the firmware volume. P14
  is **a port only** and says so.

---

## Built once, cited elsewhere

Claim a row in your chapter only if this table says it is yours. Everything else
cites the owner. `crosscheck.py` fails if a row gets two owners.

| What | Built in full in |
|---|---|
| The presence state machine and its diagram | P01 |
| The booking policy and the service axis | P02 |
| The devicetree overlay pattern and the sensor API | P03 |
| The cycle-counter measurement method | P04 |
| The out-of-tree driver, its binding and its module | P05 |
| The capture pipe and its synthetic test input | P06 |
| The flash partition overlay of the whole volume | P07 |
| The update diagram, the signature and the rollback | P08 |
| The host client for the management protocol, in C | P09 |
| The one current table, per state, on the meter | P10 |
| The message client over TLS, and the link layer | P11 |
| The second-architecture build and the wireless join | P12 |
| The certificate lifecycle, rotation and revocation | P13 |
| The two-overlay portability argument | P14 |
| The hardware test map and the framework's runner | P15 |
| The extensible SDK and its CI consumer | P16 |
| First boot, factory reset and decommission | P17 |
| The standard field profile and the silent-node reason code | P18 |
| The tunnel, and what is reachable only through it | P19 |
| The protocol bridge and its replay test | P20 |

Three rows live outside this volume entirely and are cited, never rebuilt: the
self-hosted runner and the console fixtures (firmware volume chapter 12, Linux
volume Project 4), the standard SDK (Linux volume Project 1), and the overlay on
`/etc` with its data partition (Linux volume Project 19).

---

## The idiom each chapter proves

The `Portfolio evidence` subsection names **one** idiom and the command that
proves it. A chapter that lists all eight has named none. `lint.py` checks that
the subsection contains a command.

| Idiom | What it means here | Chapter that proves it |
|---|---|---|
| Efficiency | Fixed windows, integer or Q15, no allocation after init, stacks sized from the high-water mark | P06 |
| Sustainability | Energy, not a slogan: the radio off, the timers logged, the meter in series | P10 |
| Performance | The rate, the window and the dropped-sample count, by the cycle counter | P04 |
| Reliability | Confirm then revert, and a power cut during the write | P07, P08 |
| Portability | One application, two overlays | P14 |
| Observability | One log format, counters, and a shell command that prints them | P11 |
| Maintainability | Devicetree owns the pins; the application does not | P03, P15 |
| Validation | A method and its falsifier written before the first run | P15 |

---

## Chapter structure

Twenty subsections, in this order, each `\subsection*{}`. `lint.py` enforces the
list and the five figures.

1. Why this project
2. Prior art and what to reuse (table with a licence column)
3. Parts from the inventory
4. System architecture (`zNN_arch`)
5. Configuration (devicetree and Kconfig, or services and files)
6. Wiring (`zNN_wiring`, circuitikz)
7. Memory and timing budget (`zNN_mem`, with a Measured column)
8. Software design, UML (`zNN_uml`)
9. Data flow, ASCII (`asciiart`, 112 columns, no box-drawing characters)
10. Repository layout
11. Steps (numbered, real commands)
12. Build, flash and debug
13. Verification and acceptance criteria
14. Variants
15. Pitfalls
16. Best practices applied
17. Stretch goals
18. Roadmap and next steps
19. Portfolio evidence (one idiom, one command)
20. Sources

Open the chapter with `\begin{spine}` when it owns or borrows a spine behaviour,
and with `\begin{plus}` for P18, P19 and P20. Use `\begin{honesty}` wherever the
chapter's claim has a boundary a reader would otherwise have to find.

Target 350 to 450 lines of LaTeX per chapter. Five figures: `arch`, `wiring`,
`uml`, `mem`, `timing`.

The LaTeX subset is the one `main.tex` defines. Forbidden: `\section`,
`\footnote`, `\includegraphics`, `minipage`, `\newcommand`, `\input`, `\cite`,
unicode arrows, en and em dashes.

---

## Confirm before writing

Every item here is a fact this volume does not yet hold. Write it as a question
for the bench, never as an assertion, until it is checked.

1. This part is reference manual **RM0455**, not the RM0433 part most material
   is about. No clock tree, linker script or register sequence is inherited from
   the popular sibling without being checked.
2. The board's upstream description enables neither I2C nor SPI nor the CAN
   controller, and defines no flash partitions. Every chapter that needs one adds
   an overlay and says it did.
3. There is no Ethernet on the microcontroller board. Every network path goes
   over its serial port to a Linux host, or over a radio.
4. Only one shield at a time on the microcontroller board. Only one 40-pin
   expansion board per Linux host. The small Linux board takes none of them, and
   the industrial sensor node is a USB device, not a shield.
5. The virtual console is a serial port at 115200; its pin pair is a convention
   of this board family rather than a checked fact.
6. Option bytes are not touched by any chapter until recoverability is settled.
7. The power meter is both supply and ammeter. The acquisition board resolves an
   edge to about ten microseconds on one channel and is the only witness for a
   timing claim.
8. The simulated build target is documented for x86 hosts. It runs on the
   continuous integration runner, not on the small Linux boards.
9. The modem driver upstream names a different member of the modem family than
   the one on this bench. The command differences are recorded, not hidden.
10. Four parts arrive on Monday 5 October 2026. Until each is on the bench its
    figure draws it dotted and its measurements read `not measured`.
