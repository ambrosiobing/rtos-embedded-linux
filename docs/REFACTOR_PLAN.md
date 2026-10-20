# The refactor: every chapter anchored to a published claim it can miss

Written Tuesday 20 October 2026, approved as the plan by Joseph the same day, and **nothing in the
chapters is rewritten until this page has been read.** It records what stays fixed, the rule every
chapter gets, the mapping for all twenty, the three rows that needed a confirmation, and the order
of work.

## Why

The volume's first four chapters each rest on prior art: a table of sources, what each gives and
what it does not, and acceptance criteria with a refutation condition. From P06 on, the chapters
were written as plans with a mechanism but without a published number they could fall short of,
and some of the bench work on P05 drifted into exercises that tested nothing anyone had claimed.
Joseph's instruction was to refactor the collection so that every project is about the real-time
operating system, grounded in published research, theses or repositories with standing, and built
so that it replicates and can challenge a stated result. This page is that refactor, as a plan.

## What stays fixed

- **Twenty chapters, twenty projects, five figures each.** Continuous integration enforces it and
  the structure is not the problem.
- **The four behaviours and the product spine.** A room that knows whether it is in use, and the
  four chapters that build each behaviour once.
- **Fifteen RTOS on a microcontroller, three Linux on a single-board computer, two on the
  boundary.** Embedded Linux is the largest cluster in the postings and the Linux third stays.
- **FreeRTOS and Zephyr only**, per the decision of Saturday 3 October 2026 from the count of
  twenty-three postings. P01 and P02 carry QNX adapters built before that decision; they stay as
  built and are not extended.
- **P01 to P05 keep their code and their measurements.** They gain anchors, not rewrites. A
  measured chapter is not rewritten; the missing prior-art row is added and the numbers stay.

## The rule every chapter gets

Each chapter names three things, in its prior-art table and its acceptance criteria:

1. **The mechanism**: which part of the operating system the chapter exercises. Not a sensor, not
   a protocol in the abstract, but the kernel object, subsystem or driver model that does the work.
2. **The anchor**: a peer-reviewed paper, a thesis, a standard, a vendor's documented figure, or a
   repository with standing, named with enough to find it.
3. **The replicated claim**: one number or behaviour the anchor states, which the chapter measures
   on this bench, with the figure that would refute it written before the run.

A claim without a published number it can miss is not a replication. This is the rule the first
four chapters already follow, applied to the rest.

## The volume-level thread: replicate and challenge the published kernel numbers

Four independent sources put numbers on FreeRTOS against Zephyr, and P04 already has both kernels
measured on this board, the STM32H7A3 at 280 MHz. The thread through the volume is to state P04's
figures against theirs, and to let P06 and P10 extend the comparison to messaging and power.

| Source | What it claims | Where it meets this volume |
|---|---|---|
| UL Solutions, RTOSBench on an STM32F4 at 168 MHz | interrupt latency about 101 cycles FreeRTOS against 143 Zephyr; context switch about 223 against 524 | P04, which has the same two quantities under both kernels |
| iIRID 2024, a comparative analysis of FreeRTOS and Zephyr | the peer-reviewed form of the same comparison | P04 |
| Beningo, the 2024 RTOS Performance Report, Thread Metric suite | scheduling, allocation, messaging and synchronisation costs across four kernels | P04 for scheduling and synchronisation; P06 for messaging |
| Zephyr issue 79744, "Discussion on Zephyr's poor real-time performance" | an open claim against the kernel, with the in-tree `tests/benchmarks/latency_measure` as the instrument | P04 and P05, both of which can run that benchmark on this board |

What this is not: a claim that one kernel is faster. It is four published numbers, one board that
has measured both, and a chapter that says where the figures agree and where they do not, with the
cycle counter and the acquisition board as the instruments P04 already justified.

## The mapping

| # | Chapter | Keep or reframe | Mechanism | Anchor | Replicated claim, and what refutes it |
|---|---|---|---|---|---|
| 1 | [Presence](../chapters/01-presence.md) | keep, measured | one state machine, three kernel adapters, one message queue | the volume's own three-language, three-kernel comparison | already met: the table passes in all three; refuted by any row differing between kernels |
| 2 | [The claim](../chapters/02-the-claim.md) | keep, measured | the same cascade across kernels | as P01 | as P01 |
| 3 | [One sensor](../chapters/03-one-sensor.md) | keep, measured, gains anchor | devicetree, bindings, the sensor API | Zephyr's sensor subsystem documentation; the object-identity check already written | the application object identical across two buses but for one devicetree ordinal; refuted by any instruction differing |
| 4 | [Kernel primitives](../chapters/04-what-the-kernel-primitives-cost.md) | keep, measured, gains anchor | threads, queues, work queues, priority inheritance under both kernels | the four sources above | P04's twenty-six rows stated against the published cycle counts; refuted where this board's ratio between kernels contradicts theirs without a stated reason |
| 5 | [The zones](../chapters/05-the-zones.md) | keep, measured, gains anchor | an out-of-tree driver, its binding, and the interrupt path that criterion 4 asks for | `latency_measure` for the interrupt-to-thread figure; Sensors 2026 and the ETH nano-drone work cited for the part, not replicated | data-ready as an interrupt against polling: edge-to-thread latency by cycle counter, CPU time from thread runtime statistics; refuted if the interrupt path costs more CPU than polling or its latency exceeds the benchmark's by more than the board ratio explains |
| 6 | [Capture that decides what to keep](../chapters/06-capture-that-decides-what-to-keep.md) | **reframe** | message queues, work queues and ring buffers under load; FreeRTOS queue against `k_msgq` | Beningo's message-handling test; Zephyr in-tree benchmarks | the report's messaging cost per operation; refuted if this board's figure departs from the report's ratio between kernels |
| 7 | [Settings that survive a power cut](../chapters/07-settings-that-survive-a-power-cut.md) | keep | NVS and ZMS, flash write timing, recovery after a cut mid-write | LittleFS, 7.0k stars, and its design document; Zephyr NVS | LittleFS's own power-loss test, replicated with resets mid-write; refuted by any lost or corrupted record |
| 8 | [A signed image](../chapters/08-a-signed-image.md) | keep | the bootloader, slot swap, confirm and revert | MCUboot, 2.2k stars, and its design document | swap-move against swap-scratch timing, and signature verification cost by cycle counter; refuted if a revert does not restore the previous image |
| 9 | [Fleet update](../chapters/09-fleet-update-and-the-fleet-shadow.md) | **reframe to the device side** | mcumgr over serial, chunked transfer, the slot swap | MCUboot and Zephyr's mcumgr | swap time against image size as the design document states it; refuted if time is not linear in image size |
| 10 | [A radio that sleeps](../chapters/10-a-radio-that-sleeps.md) | keep | power management: FreeRTOS tickless idle against Zephyr's PM subsystem on one board | the Power Profiler Kit II on the bench; the datasheet's sleep currents | datasheet currents in each state; refuted if the measured floor exceeds the datasheet by more than the board's own fixed loads explain |
| 11 | [Cellular from the RTOS](../chapters/11-cellular-from-the-rtos.md) | keep | the modem subsystem against raw AT commands | Zephyr's modem subsystem; the SIM7600E-H on the bench | attach time and bytes per message; refuted if the subsystem path costs more bytes than the AT path for the same payload |
| 12 | [Wi-Fi on a second architecture](../chapters/12-wi-fi-on-a-second-architecture.md) | keep, **hardware confirmed** | the same client on a second instruction set | Zephyr's ESP32 support; the SBC-NodeMCU-ESP32 on the bench, Xtensa | P11's client runs unmodified; refuted by any source change |
| 13 | [Device identity](../chapters/13-device-identity.md) | keep, **anchor made concrete** | the operating system's credential storage, mbedTLS, the X.509 lifecycle | RFC 5280 for validity and revocation; Zephyr's TLS credential API; the chapter's own rule that renewal begins with a third of validity left | a revoked certificate is refused by the broker and an expired one by the unit; refuted if either is accepted, or if renewal starts later than the rule |
| 14 | [The same application on a second board](../chapters/14-the-same-application-on-a-second-board.md) | keep | portability across two microcontroller families | P03's object-identity method generalised; the STWIN.box on the bench | the application object identical across boards; refuted by any instruction differing |
| 15 | [Twister on hardware](../chapters/15-twister-on-hardware.md) | keep as is | the framework's test runner with a hardware map | in-tree, replicable by definition | the runner's own verdict on this board, on every push |
| 16 | [The extensible SDK](../chapters/16-the-extensible-sdk.md) | keep, Linux, **edition named** | eSDK and devtool, consumed outside the build | the Yocto Project documentation for **scarthgap, 5.0 LTS**, the edition the sibling Linux volume builds with; Bootlin's public training material against the same release | a recipe modified through devtool reaches the image; refuted if the eSDK build differs from the full build |
| 17 | [First boot](../chapters/17-first-boot.md) | keep, Linux, same edition | first boot, factory reset, decommission | Yocto scarthgap; NIST SP 800-193 for the resiliency framing | the standard's protect, detect and recover clauses, each demonstrated; refuted by any clause with no demonstration |
| 18 | [CANopen on a real wire](../chapters/18-canopen-on-a-real-wire.md) | keep | a field protocol on CAN under both kernels | CANopenNode, 2.0k stars, and CiA 301 | heartbeat and NMT timing on the wire with the MCC 118 as witness; refuted if the measured period departs from the configured one by more than the witness's resolution |
| 19 | [A tunnel as the management plane](../chapters/19-a-tunnel-as-the-management-plane.md) | keep, Linux | reachability and revocation through a tunnel | WireGuard, which has a peer-reviewed paper of its own | handshake behaviour and refusal of a revoked peer; refuted if a revoked peer completes a handshake |
| 20 | [A bridge between two message protocols](../chapters/20-a-bridge-between-two-message-protocols.md) | keep | at-least-once delivery across a boundary | the MQTT specification's QoS 1; CoAP, RFC 7252; Zephyr's samples | duplicates under induced loss, none lost; refuted by any message lost |

## The three rows that needed a confirmation, and what settled them

**P12.** The bench inventory lists the SBC-NodeMCU-ESP32, so the second silicon exists and the
chapter can be written. The ESP32 needs Espressif's binary blobs, which arrive the way P05's
firmware image does and are recorded the same way.

**P13.** A TF-M and PSA anchor was proposed and was wrong for the board the chapter names: the
Cortex-M7 has no TrustZone, and TF-M needs it. The chapter's existing anchor stands and is now
concrete. The STWIN.box in P14 is a Cortex-M33 with TrustZone, so TF-M isolation is named there
as the stretch goal and not here.

**P16.** No Yocto edition was named anywhere in the volume. The sibling Linux volume builds with
scarthgap, Yocto 5.0 LTS, and P16 and P17 take the same edition so the catalogue has one.

## The order of work, and what each step delivers

1. **This page**, in front of Joseph. Nothing below starts before it has been read.
2. **A lint rule**, so the refactor is checkable: every chapter's prior-art table carries a row
   whose first cell reads *The anchor*, and its acceptance criteria carry one item beginning *The
   replicated claim*. The rule is written first so that chapters are brought level against it
   rather than against memory of this page.
3. **P04 and P05's anchor rows**, since both are measured and the thread above depends on P04.
4. **P06 onward, in order.** Each chapter's prior-art table, acceptance criteria and sources are
   reworked to the rule; the rest of the chapter's twenty-part shape is touched only where the
   mechanism changed, which is P06 and P09.
5. **P03's anchor row**, which is small and waits only because P04 and P05 come first.

Each step is one commit, run through every gate, and the chapter's figures are regenerated only
where a figure's caption or labels state something the refactor changed.

## What this plan does not do

It does not add a runtime. It does not rename a chapter, since the filenames carry the structure
continuous integration checks. It does not touch a measured number. And it does not write a
chapter whose hardware is not on the bench; the inventory is the record of that, and P12's row
changed from "depends on hardware" to "keep" by reading it rather than by assuming.
