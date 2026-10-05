# Appendix

## What this volume does not contain

A volume of twenty chapters is as much a set of decisions about what to leave out as a set of things built. This appendix records the omissions, because an omission that is not written down reads as an oversight, and because a reader deciding whether this volume is useful to them needs the boundary more than the contents list.

### Built elsewhere, and cited rather than repeated

Four sibling volumes exist on the same bench, and a great deal of what a reader might expect here is in one of them. Each row names where, so that a reader following a thread does not have to guess.

| What | Where it is built in full | Cited by |
| --- | --- | --- |
| The image for the gateway, and the standard software development kit | The embedded Linux volume, Project 1 | P16 |
| Console fixtures and a network-boot arrangement for hardware in the loop | The embedded Linux volume, Project 4 | P15 |
| A cellular router with failover and position | The embedded Linux volume, Project 15 | P19 |
| An authority, and certificates issued once for a broker | The embedded Linux volume, Project 18 | P13 |
| The Linux update mechanism: two root filesystems, a read-only root, a watchdog, the overlay on the configuration directory | The embedded Linux volume, Project 19 | P08, P13, P17 |
| The self-hosted runner, its workflow, the flashing step and the console harness | The firmware volume, chapter 12 | P15 |
| Per-phase charge accounting with marker pins, bare metal | The firmware volume, chapter 10 | P10 |
| The interrupt-to-thread latency distribution, over a million events | The firmware volume, chapter 20 | P04 |
| The hardware buffer, its watermark, and where a classifier runs | The firmware volume, chapters 13 and 16 | P06 |
| A field bus brought up in loopback, and a protocol designed rather than adopted | The robotics volume, chapters 9 to 13 | P18 |
| The industrial sensor node running its vendor firmware, as a USB device | The kit volume, lab 4 | P14 |
| The wireless microcontroller module driven from a Linux host | The kit volume, lab 16 | P12 |

*Table 1. Twelve things a reader might look for here and will find elsewhere. Each is cited from the chapter named rather than described twice, because two descriptions of one mechanism drift apart and then disagree.*

### Not built anywhere, and the reason

| What | Why not |
| --- | --- |
| A battery life in years | The arithmetic from a current to a number of years needs a duty cycle observed over a long period, and nothing on this bench has observed one. P10 publishes charge per phase instead, and its reduction script refuses to print a life |
| A ticketing client on a device | The device's job ends at a state, a reason code and an identifier. A person decides to send an engineer |
| A prediction of service life | A rising number is a reason code. The step from there to a forecast needs data this bench does not have |
| An automatic order for a spare part | The same argument, with a cost attached to being wrong |
| Exactly-once delivery | It needs a transaction across two systems that do not offer one, or a store of identifiers with a retention policy that is a deployment question. P20 sketches it with its cost |
| Conformance to the field bus profile | One master is interoperation. A certificate is a different thing and the word is not used |
| Provable erasure of storage | On flash with wear levelling, overwriting a file does not reliably overwrite the blocks that held it. P17 states the limit and relies on revocation, which is what actually ends a credential's usefulness |
| Key custody for a signing authority | An organisational problem rather than a firmware one. P08 and P13 both say so where a reader will see it |
| A tunnel on a microcontroller unit | Not attempted on a part of that size. P19 is a Linux chapter and says so |
| A fleet deployment mechanism for Linux units | P09 does this for firmware images. The Linux equivalent is a chapter this volume does not contain |
| The industrial sensor node's wideband vibration work | The firmware volume is committed to that subject, and P14 is a port that deliberately stays out of it |

*Table 2. Eleven things that are not in this volume at all. Six of them are refusals to claim something, which is a higher proportion than most books would admit to and is the honest shape of a portfolio built on one bench.*

### Measured, and not measured

Every chapter carries a budget table whose Measured column reads `not measured` wherever a run has not happened. On Friday 2 October 2026 that is most of them, and the appendix states it in one place so that nobody has to count.

Three chapters cannot be automated by the suite of P15 at all, for three different reasons: one needs the power meter in series, one needs a cellular network, and one needs a person to look at an indicator. P15 lists them in its own result on every run rather than quietly covering the other seventeen.

Four parts arrived, or were due to arrive, on Monday 5 October 2026: a field-bus expansion board, a transceiver board, an isolated interface board and a single-zone ranging breakout. Until each is on the bench, the figures draw it dotted and every measurement that would cross it reads `not measured`. P03 and P18 are the two chapters affected.

## The four behaviours, and where each is built

| Behaviour | Built in | Everything else that uses it |
| --- | --- | --- |
| Presence, with a hold and a release | P01 | P02 decides a claim from it, P05 gives it a real sensor, P08 refuses to restart while it reports a room in use, P09 publishes it |
| A sample that is allowed to leave the device | P06 | P05 supplies the zones it reduces, P11 carries what survives, P10 prices it in charge, P14 rebuilds it for a second board |
| A signed image that rolls back | P08 | P07 provides the partitions, P09 delivers the image, P13 provides the key that signs it |
| A radio that sleeps | P10 | P11 builds the link whose timers it logs, P06's byte budget becomes charge here, P04 supplies the timing method |

*Table 3. The product spine. A reader with time for four chapters should read these four in this order; the other sixteen are each about making one of them real, portable, provable or maintainable.*

## One idiom per chapter

| Idiom | Chapters | What it meant in practice |
| --- | --- | --- |
| Efficiency | P06 | Fixed windows, integer arithmetic, no allocation after start, and a stack sized from its measured high-water mark |
| Sustainability | P10 | Energy rather than a slogan: the radio off between payloads, the granted timers logged, and no number multiplied into a life it cannot support |
| Performance | P04 | The rate, the window and the statistic stated, each number naming its instrument, and each instrument's floor stated beside it |
| Reliability | P07, P08, P13, P17 | Confirm then revert, a deadline that does not depend on the thing it guards, renewal that fails safely, and a power cut during a write a thousand times |
| Portability | P12, P14 | One application, two targets, with every file that had to change listed and the one thing that could not be ported named |
| Observability | P09, P11 | One log format, a counter for every outcome that matters, and a fleet view that carries the age of every answer |
| Maintainability | P03, P05, P16 | The devicetree owns the pins, the vendor code is pinned with its licence beside it, and a device's software is recipes rather than what somebody installed |
| Validation | P02, P15, P18, P19, P20 | The method and its falsifier written first, and the checker itself proven by being made to fail |

*Table 4. The eight idioms, and the twenty chapters that each prove one. A chapter that claimed all eight would have claimed none, which is why the authoring guide allows exactly one per evidence box.*

## Reading this volume along one thread

Four threads run through the twenty chapters and each is worth following on its own.

**What the device decides** is P01, P02, P05 and P06: presence, the claim when a calendar disagrees with it, the sensor that supplies it, and the decision about what deserves to leave. A reader who wants to know what this volume is about should read these four and stop.

**What the device accepts from outside** is P07, P08, P09 and P13: where settings live, a signed image that rolls back, the delivery path over a wire that is not a network, and the identity that makes a signature mean something.

**What it costs** is P04, P10 and P06: the kernel primitives priced against each other, the one current table of the volume, and the byte budget that turns into charge when the two meet.

**Whether it survives being changed** is P03, P12, P14, P15 and P16: the boundary that makes the application portable, two demonstrations that it is, the suite that keeps the claims true, and the build system that lets a gateway be rebuilt rather than remembered.

The three chapters at the end, P18, P19 and P20, are not on any of those threads and open by saying so. Each was written because a requirement list named it, and each found one place where it genuinely contributes: a silent node becomes a specific reason code, a revocation gets a second checker so that its delay can be measured, and a delivery guarantee has to be stated precisely enough to test. A reader short of time can skip all three without losing the argument of the book.

---

[Previous](20-a-bridge-between-two-message-protocols.md) &nbsp;&nbsp;|&nbsp;&nbsp; [Contents](../README.md)
