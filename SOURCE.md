# Prior art pool

Every dependency names its licence before anything is written around it, because
finding out afterwards is expensive. Every row carries the date it was read, in
full. A row that has not been read yet says so rather than being quietly
implied.

Nothing in this file is a substitute for the reference manuals. Where a source
and a manual disagree, the manual wins and the chapter says which page.

---

## 1. The operating system and its subsystems

| Source | Gives | Does not give | Licence | Read |
|---|---|---|---|---|
| The project's board page for the Cortex-M7 board | The upstream board definition and the list of supported features: interrupt controller, serial, pin multiplexing, general-purpose pins, pulse width modulation, analogue input, backup memory, high-speed USB, random numbers | I2C, SPI and the CAN controller are **not** in that list. Every chapter that needs one adds an overlay and confirms it against the device-tree source | Apache-2.0 | Friday 2 October 2026 |
| The project's board page for the industrial sensor node | A second supported board on a Cortex-M33, 768 KiB of memory, 2 MiB of flash, its radio behind a serial host-controller binding, and its flashing paths | Which of the two flashing paths this bench can use; the chapter names the one it used | Apache-2.0 | Friday 2 October 2026 |
| The project's board page for the wireless microcontroller module | A third supported board on a different instruction set, used to show one application built twice | The binary hardware support package must be fetched separately before the first build | Apache-2.0 | Friday 2 October 2026 |
| Device-tree bindings and the sensor interface documentation | The binding grammar, the channel and trigger model, and how an overlay replaces a bus without touching the application | Any statement about which of two buses a particular shield is strapped for. That is a bench question | Apache-2.0 | Friday 2 October 2026 |
| The settings and non-volatile storage subsystems | A key-value store with a versioned schema and a documented write path | A guarantee about what a power cut during a write leaves behind on this particular flash. P07 measures it | Apache-2.0 | Friday 2 October 2026 |
| The power management documentation | Device runtime management and the idle policy, with the distinction between a device's own state and a system state | Confirmation that system states exist for this microcontroller family in the tree that is pinned. P10 checks before it claims | Apache-2.0 | Friday 2 October 2026 |
| The networking stack: the point-to-point link layer, the secure sockets layer, the message client | A link layer that turns a serial port into a network interface, and a client small enough for the part | A working configuration for this board. The chapter writes the configuration fragment and prints it | Apache-2.0 | Friday 2 October 2026 |
| The modem subsystem | A driver for a named member of the modem family, its command script and its link layer attachment | Support for **the exact module on this bench**, which is a different member. P11 records the command differences rather than hiding them | Apache-2.0 | Friday 2 October 2026 |
| The bus API and the simulated build target's host bus driver | A standard interface to the field bus, and a way to run it against a host interface with no silicon | The host driver expects a specific interface name; the chapter uses that name rather than the one most tutorials use | Apache-2.0 | Friday 2 October 2026 |
| The test runner and the unit test framework | A hardware map, a device-testing mode, and an assertion library that runs on the target | A rig. The runner and the console fixtures come from the sibling firmware volume | Apache-2.0 | Friday 2 October 2026 |

## 2. The bootloader and the update path

| Source | Gives | Does not give | Licence | Read |
|---|---|---|---|---|
| The bootloader project and its signing tool | Two slots, a signature check, the test-then-confirm sequence and the revert, plus key generation | A partition layout for this board, which defines none upstream. P07 introduces the overlay the whole volume shares | Apache-2.0 | Friday 2 October 2026 |
| The simple management protocol and its device-side server | Image upload, image list, confirm and reset, over a serial transport | **A host client in C.** The tooling upstream is in another language, so P09 writes the client this volume needs | Apache-2.0 | Friday 2 October 2026 |
| The build-system wrapper that builds application and bootloader together | One command that produces both images and keeps their configurations consistent | Any opinion about what a signed image means for a device that is in use. That gate is P08's, and it reads P01's state | Apache-2.0 | Friday 2 October 2026 |

## 3. The field bus profile

| Source | Gives | Does not give | Licence | Read |
|---|---|---|---|---|
| The open implementation of the standard device profile, carried as a module | An object dictionary, network management, heartbeat, service data objects and process data mapping, with a sample that builds | A transceiver. Until Monday 5 October 2026 the parts are drawn dotted, and the chapter says exactly which claims the loopback cannot support | Apache-2.0 | Friday 2 October 2026 |
| The host-side Python stack for the same profile | A master that can read a dictionary and drive the node, so the node is tested against something that is not itself | Any guarantee of conformance. The chapter tests interoperation with one master, which is not a certificate | MIT | Friday 2 October 2026 |

## 4. The Linux side

| Source | Gives | Does not give | Licence | Read |
|---|---|---|---|---|
| The build system's manual, software development kit chapters | The standard and the extensible kit, the development tool workflow, and how a recipe for an application is written | The image itself, which the sibling Linux volume's Project 1 owns and which P16 cites | CC-BY-2.0-UK (manual) | Friday 2 October 2026 |
| The message broker on the gateway | A broker with transport security and per-client certificates | Certificates. Those are P13's, and the broker is configured to demand them | EPL-2.0 | Friday 2 October 2026 |
| The second broker and its C client library | A different protocol with exchanges and acknowledgements, and a client that can be linked into a bridge | At-least-once semantics across the boundary. The bridge has to establish that itself, which is P20's acceptance test | MPL-2.0 (broker), MIT (client) | Friday 2 October 2026 |
| The tunnel daemon and its certificate tooling | A management plane, a certificate authority layout and a revocation list | A reachable server behind a home router. The steps say which port must be forwarded | GPL-2.0 (daemon) | Friday 2 October 2026 |

## 5. The sensor driver

| Source | Gives | Does not give | Licence | Read |
|---|---|---|---|---|
| The vendor's ultra-lite driver for the multizone ranging sensor, in C | A complete device driver: firmware upload, configuration, and a 64-zone result structure, with a reference port for Linux | Any integration with this operating system's sensor interface. P05 writes the binding, the configuration and the module wrapper, which is the chapter | Vendor licence, permissive with attribution; the text is reproduced in the project directory | Friday 2 October 2026 |
| The single-zone ranging sensor arriving Monday 5 October 2026 | A second device on the same bus, to show two devices under one overlay | Confirmation that a binding exists upstream for this exact part. P03 checks on the day and says what it found | Apache-2.0 if upstream | not yet read |

## 6. Three papers, and why none of them is the design

Each is cited by the chapter named. Each row carries the reason the chapter does
not simply follow it, because a citation without that sentence is a false one.

| Paper | Cited by | Why it is not the design |
|---|---|---|
| El Jaouhari and Bouvet, *Secure firmware over-the-air updates for IoT: survey, challenges, and discussions*, Internet of Things, 2022, doi 10.1016/j.iot.2022.100508 | P08, P09 | It classifies update threats, rollback among them, and the case of a device with no direct network reached through a gateway, which is exactly the serial path here. It is a survey rather than a measured implementation on this part, and it specifies no slot layout. The threat list is cited; the revert is measured here |
| Zhang, Xi, He and others, *A survey of mmWave-based human sensing*, IEEE Communications Surveys and Tutorials, 2023, doi 10.1109/COMST.2023.3298300 | P05 | Its framing is the one worth borrowing: a camera fails on privacy and on lighting, and a zoned sensor does not. The part on this bench is a time-of-flight array and not a 60 GHz radar, and the survey is about sensing methods rather than drivers. P05 carries that sentence of difference, without which the citation would be false |
| Andres-Maldonado, Ameigeiras, Prados-Garzon, Navarro-Ortiz and Lopez-Soler, energy modelling of a low-power cellular link with its two sleep mechanisms, Computer Networks, 2023, doi 10.1016/j.comnet.2023.109855 | P10, P11 | It is the model this volume does not copy. Its point is that a long life on a small cell follows from the link timers being chosen rather than accepted. The modem here is a different module on an expansion board, the meter is the one on this bench, and the paper's result is analytic. The number in P10 is this bench's or it reads `not measured` |

Two further candidates were considered and left out, which is recorded so the
decision is not made twice: a 2025 review of privacy in building sensing, which
is about building sensors in general rather than about the gate this volume
builds, and a 2026 laboratory study of firmware updating, whose finding is that
many devices do not update at all. Neither is closer to this bench than the
three above.

---

## Licence categories, and what each one means here

- **Apache-2.0, MIT, MPL-2.0, EPL-2.0, BSD.** Usable in a permissively published
  portfolio. Attribution is kept in the project directory, not only in this file.
- **GPL-2.0.** Used as a program on the Linux side, configured rather than
  linked. No chapter links against it.
- **Vendor licences.** Read in full before the code is vendored, and the text is
  committed beside the code it covers.
- **Anything stronger, and any course material.** Read, never pasted. One
  well-known teaching framework for state machines falls here, and P01 says so
  plainly: its ideas are worth the reading time, its code cannot go into a
  portfolio published under a permissive licence.
