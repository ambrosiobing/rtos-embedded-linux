# P09. Fleet update and the fleet shadow

> **Target:** NUCLEO-H7A3ZI-Q over its serial port to a Raspberry Pi gateway  
> **Theme:** Delivery over a wire that is not a network

> [!NOTE]
> **Where this sits in the product spine**
>
> This chapter borrows two spine behaviours and builds neither. **A signed image that rolls back** is P08's and is cited; **presence with a hold and a release** is P01's and is what the shadow publishes. What this chapter adds is everything between a new image existing somewhere and a unit running it, plus the view that lets somebody at a distance tell a quiet room from a broken one.

> **Key facts**
>
> - **Board:** NUCLEO-H7A3ZI-Q, and a Raspberry Pi 4 as the gateway
> - **Peripherals:** One serial port between them. There is no Ethernet on the microcontroller board, and that is the premise of the chapter rather than an obstacle to it
> - **Toolchain:** As the front matter, plus the upstream management client on the gateway, since the protocol has several and this chapter's work is on the unit
> - **Operating system:** The device application of P08 with the management server on its own work queue, receiving chunks and writing a slot while P01 keeps running; a broker, an image store and the upstream client on the gateway
> - **Difficulty:** 5 of 5
> - **Effort:** 5 evenings of about four hours
> - **Deliverable:** An image that travels from a store to a unit over a serial port, with the upload and the swap timed against image size and the slopes predicted first, and P01's period held through the upload; a published state carrying a sequence number and a staleness flag; and a shadow on which P02's cases run against a fleet rather than against one board

## Why this project

P08 made an image safe to install. It said nothing about how the image gets there, and on this board that question is sharper than usual because there is no network interface at all. The device's only route to the world is a serial port, so something at the other end of that port has to be a full participant: it subscribes to an announcement, fetches an image, pushes it over the serial link, and reports what happened.

That shape is not a workaround. It is what a connected device with a gateway actually looks like, and the published survey of update threats treats it as a first-class case: a device reached only through an intermediary, where the intermediary is trusted to deliver but not to decide. The signature is what keeps the intermediary from being trusted to decide, which is why P08 comes first.

The second half is the view. A fleet of units that each know whether their room is in use is useless unless somebody can ask them, and the hard part of asking is not the question but the honesty of the answer. A unit that has been silent for an hour must not be reported as free; it must be reported as last seen free an hour ago, and that distinction is the whole of the shadow.

**What the operating system contributes, and what this chapter measures, is on the unit.** The management server receives each chunk on the serial transport, decodes it on its own work queue thread, writes it into the second slot through the stream flash layer, and answers with the offset it has reached, which is what makes a resumed upload possible at all. It does that while P01's state machine is running, and whether P01's period survives an upload is a question about thread priorities and flash programming stalls that only this board can answer. The gateway's client is upstream's; the chapter's contribution is the server's behaviour under load, measured.

**The published numbers this chapter can miss** are two slopes. The serial transport's framing is documented, so the time to upload an image at a given rate is a straight line in its size with a slope the encoding predicts; and the bootloader's design document says a swap moves every sector the image occupies, so the swap time is a straight line in image size with a slope P08's per-sector figure predicts. Both lines are drawn before the run and both are measured at three image sizes.

> [!NOTE]
> **What this chapter does not claim**
>
> The fleet here is one device. Everything about the shadow is written so that it works for many, and nothing about it has been tested with more than one, and the chapter says so rather than implying a scale it has not reached.
>
> The gateway's client is not this chapter's code. When this chapter was first written it said no client existed upstream in C and set out to write one; read again on Tuesday 20 October 2026, the kernel's own documentation lists five, including an in-tree C client library and a Python one for a host, and the chapter uses the upstream one on the gateway. What is written here is on the unit.
>
> The gateway is trusted to deliver and not to decide, which is a property of the signature rather than of anything in this chapter. Nothing here authenticates the gateway to the device beyond that.

## Prior art and what to reuse

| Source | What it gives | What it does not | Licence |
| --- | --- | --- | --- |
| The simple management protocol, its device-side server and its clients | Image upload in chunks with a resumable offset, image list, mark for test, confirm and reset, over a serial transport, already on the device; and on the host side the clients the kernel's documentation lists, an in-tree C library, a Python library and three applications | A figure for what the server costs the unit while it runs, which is the chapter | Apache-2.0 |
| The anchor: the protocol's image management specification, and the bootloader's design document | The image group's upload request is specified field by field: image number, total length and digest in the first chunk only, an offset in every chunk, and a response carrying the last offset written, with the rule that an offset of zero in the reply means the client must resend the first chunk's fields. That is the resumption contract, written down. The design document says a swap moves every sector the image occupies and records the swap size in the trailer, which is the sentence that makes the swap time a line in image size | A slope for this board. The encoding overhead per chunk and the per-sector swap cost are this part's to measure; the specification gives the shape of both lines and not their gradient | Apache-2.0 |
| P08, in this volume | Everything after the image has arrived: verification, swap, test, confirm and revert | Delivery, and any view of more than one device | Apache-2.0 |
| The broker on the gateway | Announcements and state, with transport security and per-client certificates | The certificates, which are P13's | EPL-2.0 |
| Published conventions for device state in a message topic tree | The shape that works: a retained state topic per device, a sequence number, and a last-seen time | A staleness policy, which is a product decision and is made here | Published |
| The survey of over-the-air update threats | The case of a device reached only through an intermediary, named and classified | A measured implementation over a serial link on this part. The threat case is cited; the delivery is built and timed here | Published |
| P02, in this volume | The twenty-two claim cases, which the shadow replays against the fleet view rather than against the policy function | Anything about transport | Apache-2.0 |

*Table 9.1. Prior art for P09. There is no gap on the host side: the protocol is specified, implemented on the device and implemented in several clients. The chapter's own work is the server's behaviour on the unit under an upload, and the two lines the anchor row lets it predict before measuring them.*

## Parts from the inventory

| Part | Role | Interface |
| --- | --- | --- |
| NUCLEO-H7A3ZI-Q | The unit. Runs P08's application with the management server enabled | Serial port to the gateway; USB for programming only |
| Raspberry Pi 4 | The gateway: broker, image store, and the upstream client | Serial port to the unit; the building network on the other side |
| Two female jumper leads, plus a ground | Transmit, receive and ground between the two boards, crossed, at 3.3 V | 3.3 V, and nothing else is shared |

*Table 9.2. Inventory for P09. No expansion board is fitted on the gateway, which keeps its header free and means this chapter can run on the same Pi that carries the acquisition board in P04 without anything being unplugged.*

## System architecture

![Figure 9.1. The announcement travels over the building network; the image travels over a serial port.](../projects/09-fleet-update-and-the-fleet-shadow/docs/figures/z09_arch.svg)

*Figure 9.1. The announcement travels over the building network; the image travels over a serial port. The gateway is a full participant rather than a cable, running the upstream client, and the signature is what keeps it from being a trusted one. The unit's management server is the chapter's subject: it receives chunks on its own work queue and writes the second slot while P01 keeps running. The shadow is the gateway's view of every unit it knows about, and it is deliberately drawn as holding a last-seen time rather than a state.*

The arrangement has one property that is easy to lose and expensive to recover. The gateway stores what a unit last said and when it said it, not what the unit is. Everything that reads the shadow therefore has to decide for itself whether an hour-old answer is good enough, and because the shadow carries the age, it can. A shadow that stored only the state would force every reader to assume it was current, and the assumption would be wrong exactly when it mattered.

## Configuration

```text
# the device's half: the management server over the serial port
CONFIG_MCUMGR=y
CONFIG_MCUMGR_TRANSPORT_UART=y
CONFIG_MCUMGR_GRP_IMG=y            # upload, list, confirm
CONFIG_MCUMGR_GRP_OS=y             # reset, and the echo used for liveness
CONFIG_MCUMGR_TRANSPORT_UART_BUF_SIZE=2048
CONFIG_BASE64=y
CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE=2048
CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_THREAD_PRIO=10  # below P01's thread, on purpose
CONFIG_THREAD_RUNTIME_STATS=y       # what the upload costs, per thread
```

```text
# the topic tree on the gateway, written down once so it is not improvised
fleet/<unit-id>/state      retained, published by the gateway for each unit
fleet/<unit-id>/health     retained, the service axis of P02
fleet/<unit-id>/image      retained, the version now running and its status
fleet/announce             an image is available: version, size, digest
fleet/<unit-id>/command    restart, update, resend state
```

The work queue priority is the one line that is a decision rather than a default. The server runs below P01's thread so that an upload cannot delay a presence transition, and the price is that a chunk waits while P01 runs, which is the right way round for a device whose job is the room. Whether the price is visible at the serial rate is one of the chapter's measurements.

The state topic is retained and the health topic is separate, which repeats on the wire the separation P02 made in the device. A reader that wants to know whether a room is free and a reader that wants to know whether the room is working are different readers with different urgencies, and putting both in one message means every change to either wakes both.

## Wiring

![Figure 9.2. Three leads between two boards, crossed, with a shared ground and nothing else.](../projects/09-fleet-update-and-the-fleet-shadow/docs/figures/z09_wiring.svg)

*Figure 9.2. Three leads between two boards, crossed, with a shared ground and nothing else. The programming cable stays attached here, unlike P07, because nothing in this chapter cuts the supply and the console is wanted on both sides at once.*

The serial link carries two things that are usually separate: the management protocol during an update, and the device's own log the rest of the time. They share the port because the board has one usable serial port to spare, and the chapter is explicit that this is a constraint of the bench rather than a design preference. The protocol framing makes the sharing safe, and the symptom when it is misconfigured is a log line in the middle of an upload, which is distinctive enough to diagnose quickly.

## Memory and timing budget

![Figure 9.3. Where an image is while it is in flight.](../projects/09-fleet-update-and-the-fleet-shadow/docs/figures/z09_mem.svg)

*Figure 9.3. Where an image is while it is in flight. It exists three times at once during an update: in the store, in the gateway's buffer as it is chunked, and in the unit's second slot. The figure makes the point that the gateway needs room for a whole image and the unit does not need room for two complete ones at once.*

| Quantity | Budget | Measured | Margin |
| --- | --- | --- | --- |
| Management transport buffer, device | 2048 B static | 2048 B by construction | none needed |
| One chunk, over the serial link | under 512 B encoded | by construction | not measured |
| Upload of a 400 kB image | under 15 min at 115200 | not measured | not measured |
| Upload, at the highest reliable rate | stated once measured | not measured | not measured |
| Upload time against image size, three sizes | a line, slope predicted from the framing | not measured | the replicated claim |
| Swap time against image size, three sizes | a line, slope from P08's per-sector figure | not measured | the replicated claim |
| P01's period during an upload | unchanged within P04's tolerance | not measured | not measured |
| Processor time of the server during an upload | stated, from thread runtime statistics | not measured | not measured |
| Image store on the gateway | two versions kept | by construction | disk, not memory |
| State publication interval | on change, and hourly | by construction | not a measurement |
| Staleness threshold | 600 s, a settings key | by construction | a product decision |
| Shadow entry per unit | under 256 B | by construction | on the gateway |
| Time from announcement to confirmed | stated end to end | not measured | the chapter's result |

*Table 9.3. The budget for P09. The two lines are the chapter's measurements with a published shape, and the period row is the one that says whether the server was worth putting below P01. The upload row is the one that shapes the chapter: at a console baud rate a four hundred kilobyte image takes minutes rather than seconds, which is why the rate is measured rather than assumed and why the restart gate from P08 matters more here than it did there.*

## Software design (UML)

![Figure 9.4. One update, end to end, with the gateway in the middle.](../projects/09-fleet-update-and-the-fleet-shadow/docs/figures/z09_uml.svg)

*Figure 9.4. One update, end to end, with the gateway in the middle. Everything after the image has arrived is P08's and is not redrawn; what this chapter owns is the upload itself as the unit sees it, chunk, write, offset, and the one decision the gateway makes, which is when to stop trying.*

On the unit the sequence is three threads and one flash. The serial interrupt fills the transport buffer; the management work queue decodes a frame, checks the offset against the one it holds, and hands the bytes to the stream flash layer, which programs the second slot in whole write blocks; P01's thread runs above both. The offset the unit holds is the whole of resumption, and it lives in the server's own state rather than in the gateway's memory, which is why a gateway that restarts can ask rather than guess.

The retry policy is the gateway's only real decision and it is written down rather than left to a loop. An upload that fails part way is resumed rather than restarted, because restarting a fifteen minute transfer on a flaky link is how a fleet never updates. Three resumptions and then the gateway stops, marks the unit in the shadow with a reason, and waits for somebody to ask. A gateway that retries forever is a gateway that hides a broken unit behind an infinite queue.

```c
/* the gateway's one decision, over the upstream client's calls */
static int deliver(unit_t *u, const image_t *img)
{
    for (int attempt = 0; attempt < 3; attempt++) {
        off_t from = smp_img_upload_offset(u);    /* resume, never restart */
        int rc = smp_img_upload(u, img, from);
        if (rc == 0) { return smp_img_test(u); }  /* mark for test, do not reset */
        LOG_WARN("unit=%s attempt=%d resumed_from=%ld rc=%d",
                 u->id, attempt, (long)from, rc);
    }
    shadow_set_reason(u, "upload_failed_three_times");  /* visible, not retried */
    return -EIO;
}
```

Marking for test and resetting are deliberately separate calls. The gateway may deliver an image at any time; when the unit restarts is the unit's decision, because only the unit knows whether the room is in use. That split is the chapter's one structural contribution to P08's scheme.

![Figure 9.5. An update across an afternoon, at a scale where the serial link is the slow part.](../projects/09-fleet-update-and-the-fleet-shadow/docs/figures/z09_timing.svg)

*Figure 9.5. An update across an afternoon, at a scale where the serial link is the slow part. The upload takes minutes, the deferral until the room is free can take hours, and the swap takes seconds. Anything watching this fleet has to be told all three, or it will report a working update as a failure.*

## Data flow (ASCII)

```text
  the building network                 the serial port, 115200, no Ethernet
  +---------------------+        +-----------------------------------------+
  | an image is         |        | gateway                                 |
  | announced:          | -----> |   fetch it, check the digest            |
  | version, size,      |        |   chunk it, encode it, push it           |
  | digest              |        |   resume on failure, three times, then   |
  +---------------------+        |   stop and say so in the shadow          |
                                 |   mark for test; do NOT reset            |
                                 +-----------------------------------------+
                                                   |
                                                   v
  +--------------------------------------------------------------------------+
  | unit: P08 takes over from here and this chapter does not repeat it        |
  |   the unit decides WHEN to restart, because only the unit knows the room  |
  +--------------------------------------------------------------------------+
                                                   |
                      state, with a sequence number|
                                                   v
  +--------------------------------------------------------------------------+
  | the shadow: per unit, the last thing it said AND when it said it          |
  |   a reader asking "is room 3 free" gets: free, as of 40 seconds ago       |
  |   a reader asking after an outage gets: free, as of 70 minutes ago, STALE |
  +--------------------------------------------------------------------------+
```

## Repository layout

```text
projects/P09-fleet/
  device/
    prj.conf                      # the management server, on its own work queue
    src/publish.c                 # state, with a sequence number, on change
    src/period_watch.c            # P01's period, sampled through an upload
    bench/sizes/                  # three signed images of three sizes, for the two lines
  gateway/
    deliver.py                    # fetch, chunk, resume, mark for test, over the upstream client
    lines.py                      # upload and swap time against size, with the predicted slopes
    shadow.c                      # last said, and when, per unit
    shadow.h
    broker.conf                   # topics, retention, and who may publish what
  tools/
    replay_cases.py               # P02's twenty-two cases, against the shadow
    announce.py                   # put an image in the store and announce it
  tests/
    test_resume.c                 # a cut link mid-upload resumes, not restarts
    test_stale.c                  # an old entry is reported with its age
  docs/topics.md                  # the topic tree, written down once
  README.md
```

## Steps

**Step 1.** **Bring the management server up on the device and talk to it from the gateway by hand.** Before writing a client, prove the transport with the tool that already exists, so that a later failure is known to be in the client rather than in the link.

**Step 2.** **Use the upstream client on the gateway, and write down the four calls the chapter needs from it.** List images, read the offset, upload from an offset, mark for test. The client is not the chapter's work and a wrapper of forty lines around it is all the gateway carries.

**Step 3.** **Predict the two lines before the first upload.** The serial transport's framing gives bytes on the wire per chunk of payload, and the rate gives seconds per byte, so the upload slope is arithmetic; P08's per-sector swap figure and this part's sector size give the swap slope. Write both down with the intercepts, then build three signed images of three sizes from the same source with padding.

```python
# gateway/lines.py: the prediction, written before the run, from the
# serial transport's documented framing: a packet carries a two-byte length
# and a CRC16, is base64 encoded, and is cut into lines of at most 127 bytes,
# each a two-byte marker, 124 characters and a newline.
PAYLOAD = 512                       # bytes of image per chunk
CBOR = 48                           # the upload request's other fields, about

def wire_bytes(packet_raw):
    encoded = -(-(packet_raw + 4) * 4 // 3)      # length, CRC16, then base64
    lines = -(-encoded // 124)
    return encoded + 3 * lines

def upload_seconds(image_bytes, baud):
    chunks = -(-image_bytes // PAYLOAD)
    return chunks * wire_bytes(PAYLOAD + CBOR) * 10 / baud   # ten bits per byte
```

**Step 4.** **Make resumption work before making the happy path fast.** A transfer that cannot resume is a transfer that never completes on a link that occasionally drops, and the happy path is the easy half.

**Step 5.** **Separate marking from resetting, and write down why.** The gateway marks; the unit chooses the moment. This is the one place where P08's restart gate becomes operationally visible.

**Step 6.** **Publish state with a sequence number, from the device.** The number is what lets a reader detect a gap. The gateway does not invent it.

```c
/* device/src/publish.c: the device numbers its own statements */
static uint32_t seq;

void publish_state(claim_t c, reason_t why)
{
    char buf[96];
    snprintf(buf, sizeof buf,
             "{\"seq\":%u,\"claim\":%d,\"why\":%d,\"up_s\":%u}",
             ++seq, (int)c, (int)why, k_uptime_get_32() / 1000U);
    transport_publish("state", buf);      /* or spool it, as P02 decided */
}
```

**Step 7.** **Make the shadow store the time, not just the state.** This is the chapter's main idea and it is two extra fields.

```c
/* gateway/shadow.h */
typedef struct {
    char     unit_id[24];
    claim_t  last_claim;
    uint32_t last_seq;
    time_t   last_heard;        /* the field that makes the rest honest */
    bool     stale;             /* derived: now - last_heard > threshold */
    char     reason[32];        /* why delivery stopped, if it did */
} shadow_t;
```

**Step 8.** **Replay P02's cases against the fleet view.** The same twenty-two sequences, driven through the gateway rather than into the policy function, with the expected claim and reason checked at the shadow. A policy that is right in a unit test and wrong through a transport is a policy nobody has actually tested.

```bash
python tools/replay_cases.py --broker localhost --unit bench-01 --cases 22
```

**Step 9.** **Sample P01's period through an upload, and read the thread runtime statistics after it.** The period watcher is P04's witness pattern on the device alone: the presence thread's period, logged while the server is busiest. A period that stretches is the server above P01, or a flash program stalling the core, and the two are told apart by moving the work queue priority.

**Step 10.** **Cut the link in the middle of an upload, deliberately.** Unplug the lead, watch the attempt fail, plug it back, and confirm the next attempt resumes from where it stopped rather than from zero.

**Step 11.** **Let an upload fail three times and confirm the unit is marked rather than retried forever.** The shadow carries a reason and somebody can see it. This is the acceptance criterion that distinguishes a fleet tool from a loop.

**Step 12.** **Stop publishing from the device and confirm the shadow ages rather than lies.** After the threshold, a reader asking about the room gets an answer with an age and a staleness flag, not a confident wrong answer.

**Step 13.** **Time the whole thing, announcement to confirmed, and write the number down.** It is minutes rather than seconds, and anything that watches this fleet needs to know that.

## Build, flash and debug

The gateway's client and the device's log share one serial port, which is the chapter's main source of confusion and its main diagnostic convenience. When an upload stalls, the first thing to look at is whether a log line arrived in the middle of it; the framing recovers, but a device logging at a high level during a transfer will make the transfer slow in a way that looks like a link problem.

The shadow is inspectable from the broker with a subscription and nothing else, which is deliberate. A fleet view that needs its own tool to read is a fleet view nobody checks, and the whole point of putting the age in the message is that a person with a terminal can see it.

## Verification and acceptance criteria

- **An image travels from the store to a running unit.** Announcement, fetch, upload, mark, restart when the room is free, confirm. *Refuted if* any step needs a person at the bench.
- **A cut link resumes rather than restarts.** The second attempt begins at the offset the first reached. *Refuted if* it begins at zero, which on this link means an update that never finishes.
- **Three failures stop, and say why.** The shadow carries a reason and the gateway does not keep trying. *Refuted if* it retries indefinitely, which hides a broken unit behind a queue.
- **Marking and resetting are separate.** The unit does not restart when the image arrives; it restarts when the room is free. *Refuted if* delivery causes a reboot.
- **The shadow reports age, not just state.** Every answer carries a last-heard time. *Refuted if* a reader can get a state without an age.
- **A silent unit goes stale rather than wrong.** After the threshold with no publication, the entry is flagged. *Refuted if* the last known state is still presented as current.
- **A sequence gap is detectable.** Dropping a publication leaves a hole a reader can see. *Refuted if* the gateway renumbers.
- **P02's cases pass through the transport.** All twenty-two produce the expected claim and reason at the shadow. *Refuted if* any differs from the unit test, and the difference is the finding.
- **The end-to-end time is stated.** *Refuted if* the chapter reports that it is quick.
- **The replicated claim: upload time and swap time are straight lines in image size, with the slopes the framing and the design document predict. Not yet run, and both lines are drawn first.** Three signed images of three sizes from one source, each uploaded three times at one rate and each swapped once. The upload slope is predicted from the serial transport's framing, a length and a checksum per frame and the encoding's four thirds, and the swap slope from P08's per-sector figure, since the design document moves every occupied sector and no other. *Refuted if* either set of nine points is not a line, which would say the server does something per image that is not per byte; or if the upload slope departs from the predicted one by more than a factor of 1.5, which would say the framing on the wire is not the framing in the documentation and the chapter reads the frames to find what else is there.
- **P01's period holds through an upload.** With the server below P01's thread, the presence period during the busiest part of an upload is within P04's tolerance of its idle value, and the server's processor time is stated from the thread runtime statistics. *Refuted if* the period stretches, and then the priority is moved to tell the server from a flash stall.

## Variants

| Axis | This chapter | The alternative | Cost | Built in full |
| --- | --- | --- | --- | --- |
| Transport | A serial port to a gateway | A network interface on the device | There is none on this board, which is the premise | Here |
| Host client | Upstream's, with a forty-line wrapper | One written here, as the chapter first proposed | Code that measures nothing, since the protocol is the server's and the server is the subject | Here |
| Where the server runs | Its own work queue, below P01 | The system work queue, or above P01 | A presence transition waits on a chunk | Here, and the period row says whether it mattered |
| Failure handling | Resume, three times, then stop | Restart, forever | Updates never complete, and failures are hidden | Here |
| Marking and resetting | Separate calls | One call | The unit restarts during a meeting | Here |
| The fleet view | Last said, and when | The state alone | Every reader assumes it is current | Here |
| What the device numbers | Its own statements | The gateway numbers them | A gap becomes invisible | Here |
| Verification of the image | Not here | Signature checked at the device | Already built | P08 |
| Who the gateway is | Not authenticated to the device | Mutual authentication | A different chapter | P13 |

*Table 9.4. Variants touching P09. Seven rows are built here. The last two are the chapter's boundary: this chapter trusts the gateway to deliver and not to decide, and the two reasons that is safe are built elsewhere.*

## Pitfalls

- **Restarting a failed upload from zero.** On a link that occasionally drops, a transfer that cannot resume is a transfer that never finishes.
- **Retrying forever.** It converts a broken unit into a busy gateway, and nobody sees the broken unit.
- **Resetting the device as part of delivery.** The room is sometimes in use, and the unit is the only part of the system that knows.
- **A shadow that stores only the state.** Every reader then assumes it is current, and the assumption fails exactly when something is wrong.
- **Letting the gateway assign the sequence number.** A gap then becomes invisible, which removes the only cheap way to detect a lost publication.
- **Logging at a high level during an upload.** The two share a port, and the transfer slows in a way that looks like a bad link.
- **Testing the policy only in a unit test.** A policy that is right in isolation and wrong through a transport has not been tested where it runs.
- **Writing a client because the one you want is in another language.** Five exist upstream. The chapter did propose it, and the client would have measured nothing about the unit.
- **Leaving the server at the default priority.** It then competes with the application for the processor during exactly the minutes the application is least able to spare, and nobody notices until a room reads wrong during an update.

## Best practices applied

The server's cost is measured on the unit, against two published shapes, with the lines drawn before the run; the client is upstream's and is used as it stands. Resumption is built before speed, because the failure mode decides whether the feature works at all. Retries are bounded and their exhaustion is visible rather than silent. The authority to decide when to restart stays with the part of the system that has the information. The fleet view stores a time alongside a state, so that staleness is a fact a reader can act on rather than an assumption. And the policy from an earlier chapter is replayed through the real transport, because that is where it will actually run.

## Stretch goals

Measure the upload at several serial rates and find the one where the error rate starts to cost more than the speed gains, which is a number this bench can actually produce. Add a second unit, even if it is the same board reflashed with a different identifier, and confirm the shadow keeps them apart. Record a week of state publications and check that a reader can reconstruct, from the sequence numbers alone, every interval during which the fleet view was not current.

## Roadmap and next steps

P13 gives the unit and the gateway identities, at which point the gateway stops being trusted by default and starts being authenticated. P11 replaces the serial link with a cellular one, and the resumption built here stops being a nicety. P02's service axis is published on its own topic from this chapter onward, which is what lets a technician see a unit that needs attention without subscribing to every room's occupancy. And P19 puts the whole management path behind a tunnel, so that the broker is not reachable from the building network at all.

## Portfolio evidence

The idiom this chapter proves is **observability**: one log format, counters that matter, and a fleet view that carries the age of every answer so that a stale one cannot be mistaken for a current one. The command that proves it is

`python tools/announce.py --image build/app.signed.bin && python gateway/deliver.py --unit bench-01 && python gateway/lines.py --sizes 3 && python tools/replay_cases.py --cases 22`

which announces an image, delivers it over the serial link with resumption, fits the two lines against their predictions, and then replays the twenty-two claim cases through the transport against the shadow. Publish the architecture figure, the retry decision, the shadow structure, a trace of a cut link resuming, and the measured end-to-end time from announcement to confirmed.

## Sources

- The simple management protocol, its device-side server and its transport over a serial port. Read Friday 2 October 2026, and again on Tuesday 20 October 2026, when the kernel's management page listed the clients that exist upstream, an in-tree C library, a Python library and three applications, which is why the one this chapter first proposed to write is not written.
- The protocol's image management group specification, read on Tuesday 20 October 2026, for the upload request's fields, the first-chunk-only fields, the offset in every reply, the rule that an offset of zero means the first chunk's fields are resent, and the state-of-images write that marks for test or confirms.
- MCUboot's design document, read on Tuesday 20 October 2026 for the swap size recorded in the trailer and the sentence that a swap moves the occupied sectors, which is what makes the swap time a line in image size. P08 carries the rest of that document.
- P08 in this volume, for everything after the image has arrived, which this chapter cites and does not repeat.
- The broker's documentation, for retained topics and for the configuration that decides who may publish what.
- El Jaouhari and Bouvet, a survey of secure firmware updates over the air for connected devices, Internet of Things, 2022, for the classification of a device reached only through an intermediary. It is a survey and not a measured implementation on this part, so the case is cited and the delivery is timed here.
- Published conventions for device state in a topic tree, read for the retained-state-plus-sequence shape rather than for a staleness policy, which is a product decision made in this chapter.

---

[Previous](08-a-signed-image.md) &nbsp;&nbsp;|&nbsp;&nbsp; [Contents](../README.md) &nbsp;&nbsp;|&nbsp;&nbsp; [Next](10-a-radio-that-sleeps.md)
