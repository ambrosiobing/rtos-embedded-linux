/* projects/01-presence/qnx/presence_adapter.c: the same contract, under QNX.
 *
 * **THIS FILE HAS NEVER BEEN COMPILED.** There is no QNX licence and no QNX target on
 * this bench, and there will not be one. Every API call here comes from the
 * documentation and none of it from a build, which is a weaker kind of evidence than
 * anything else in this project and is said plainly here rather than left for a
 * reader to discover. ../freertos/ is green and ../zephyr/ is written against a
 * toolchain that exists; this is a design, in C, that argues for itself.
 *
 * It is here because QNX is the kernel that does NOT fit, and the two places it does
 * not fit are the only interesting rows in the mapping table of
 * ../docs/RTOS_VARIANTS.md. Writing it is how those two were found, and one of them
 * changed the table every other implementation in this project uses.
 *
 * ---------------------------------------------------------------- the first misfit
 *
 * A PULSE CARRIES FOUR BYTES AND THE EVENT IS TWENTY. MsgSendPulse takes an int
 * value, and presence_event_t is a kind, a timestamp and a twelve-byte union. Three
 * ways out were considered in RTOS_VARIANTS.md and the second is taken here:
 *
 *   1. Send a message rather than a pulse. MsgSend carries any payload, and blocks
 *      the sender until a server replies, which turns every sensor reading into a
 *      rendezvous and gives the sensor thread a way to be blocked by the dispatcher.
 *      Rejected: the dispatcher would stop being the only thing deciding order.
 *   2. Keep the events in a ring this adapter owns and send a pulse as a doorbell.
 *      Taken, and the pulse carries no index: the ring already holds the order, and a
 *      pulse that repeated it would be a second description of the same thing, with
 *      no way to notice when the two stopped agreeing.
 *   3. Shrink the event to four bytes. Rejected, and it is the one that looks
 *      cheapest: a settings event carries eight bytes of payload, so the six settings
 *      rows would have to stop being events, and the table would stop being the whole
 *      specification.
 *
 * The cost of option 2 is stated rather than hidden: the queue is now this adapter's
 * and not the kernel's, so requirement 1 of the contract, one queue in order, is this
 * file's to guarantee. Under Zephyr and FreeRTOS the kernel guarantees it.
 *
 * A FOURTH OPTION EXISTS AND THE PAGE DID NOT CONSIDER IT. POSIX message queues,
 * mq_open and mq_send, carry an arbitrary payload and are available on QNX, which
 * would remove the ring entirely. It is not taken here for two reasons worth writing
 * down rather than asserting: this chapter is about the native IPC model, where a
 * pulse is the primitive a reader came to see, and an mq on QNX is served by a
 * resource manager that has to be running, which turns a self-contained adapter into
 * one with a deployment dependency. If this were production rather than a comparison,
 * option 4 would deserve the measurement that would settle it.
 *
 * --------------------------------------------------------------- the second misfit
 *
 * A CHANNEL DELIVERS BY PRIORITY, NOT IN ORDER. MsgReceivePulse returns the highest
 * priority pulse waiting, so a pulse from a high priority sensor thread can be
 * received ahead of an older one from a lower priority timer. That breaks requirement
 * 1 outright, and it is why row 17 of the table is guarded: an expiry from a hold that
 * was already cancelled could otherwise be delivered after a NEW hold had started and
 * release it up to hold_ms early.
 *
 * This adapter does both things about it. Every pulse is sent at ONE priority, so the
 * channel cannot reorder them, and the table carries the guard anyway, because a
 * single priority is a property of this file that the table should not have to trust.
 * The guard was added for this kernel and costs the other two nothing.
 */
#include "presence_adapter.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <time.h>

/* ----------------------------------------------------------------- the ring */

/* The queue option 2 obliges this adapter to own. PRESENCE_QUEUE_DEPTH entries, from
 * presence.h, so the memory budget in docs/DESIGN.md is one multiplication and cannot
 * drift from the two kernels that use a kernel queue of the same depth. */
static presence_event_t ring[PRESENCE_QUEUE_DEPTH];
static unsigned ring_head;      /* written by the dispatch thread */
static unsigned ring_tail;      /* written by producers, under the mutex */
static unsigned ring_count;
static pthread_mutex_t ring_lock = PTHREAD_MUTEX_INITIALIZER;

/* The doorbell. One channel, one connection to it, and one pulse code. The pulse's
 * value carries nothing: the ring is the queue and the pulse only says that it is not
 * empty. Putting the index in the value would make the pulse and the ring two
 * descriptions of the same order, and the one that got out of step would be silent. */
#define PULSE_CODE_EVENT  (_PULSE_CODE_MINAVAIL + 1)
#define PULSE_PRIORITY    10        /* ONE priority, which is the second misfit */

static int channel_id = -1;
static int connection_id = -1;

static timer_t hold_timer;
static timer_t tick_timer;
static bool hold_timer_created;
static bool tick_timer_created;

static pthread_t dispatch_tid;

/* The machine. One node, one context, and the dispatch thread is the only writer. */
static presence_t ctx;
static uint32_t dropped;
static presence_trace_fn trace;

#define DISPATCH_PRIORITY 20
#define PRODUCER_PRIORITY 19        /* QNX counts upwards: larger preempts */

/* ------------------------------------------------------------------ the clock */

/* One clock for every event, which is requirement 2. CLOCK_MONOTONIC in
 * milliseconds, truncated to the 32 bits the event carries, which wraps at 49.7 days
 * exactly as the other two adapters do. The guard on row 17 compares a signed
 * difference for that reason and is correct across the wrap. */
static uint32_t now_ms(void)
{
    struct timespec ts;

    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u +
                      (uint64_t)ts.tv_nsec / 1000000u);
}

/* --------------------------------------------------------------- the posting */

/* Append under the mutex, then ring the doorbell. The order matters: a pulse sent
 * before the event is in the ring would let the dispatch thread wake to an empty
 * ring, which is harmless, but a pulse NOT sent after an append would leave an event
 * sitting there until the next one arrived, which is an event delayed indefinitely.
 * So the append is first and the pulse is unconditional. */
static bool post_stamped(const presence_event_t *ev)
{
    presence_event_t stamped;
    bool appended;

    if (ev == NULL || connection_id < 0) {
        return false;
    }
    stamped = *ev;
    stamped.at_ms = now_ms();

    (void)pthread_mutex_lock(&ring_lock);
    appended = ring_count < PRESENCE_QUEUE_DEPTH;
    if (appended) {
        ring[ring_tail] = stamped;
        ring_tail = (ring_tail + 1u) % PRESENCE_QUEUE_DEPTH;
        ring_count++;
    } else {
        dropped++;
    }
    (void)pthread_mutex_unlock(&ring_lock);

    if (!appended) {
        return false;
    }

    /* One priority for every pulse, always. This is the line that keeps the channel
     * from reordering events, and changing it would silently remove requirement 1. */
    if (MsgSendPulse(connection_id, PULSE_PRIORITY, PULSE_CODE_EVENT, 0) == -1) {
        return false;
    }
    return true;
}

bool presence_adapter_post(const presence_event_t *ev)
{
    return post_stamped(ev);
}

/* On QNX this is not called from an interrupt service routine, and the name is kept
 * only because the contract is shared. The documented way to handle an interrupt here
 * is InterruptAttachEvent, which delivers an event to a thread rather than running
 * code in interrupt context, and that thread calls this. A true ISR could not take
 * the mutex this ring needs, which is the third place QNX differs from the other two
 * and the reason option 2's cost is worth restating: owning the queue means owning
 * its concurrency. */
bool presence_adapter_post_from_isr(const presence_event_t *ev)
{
    return post_stamped(ev);
}

uint32_t presence_adapter_events_dropped(void)
{
    return dropped;
}

const presence_t *presence_adapter_context(void)
{
    return &ctx;
}

void presence_adapter_set_trace(presence_trace_fn fn)
{
    trace = fn;
}

void presence_adapter_sleep_ms(uint32_t ms)
{
    struct timespec ts;

    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    (void)nanosleep(&ts, NULL);
}

void presence_adapter_test_outrank_dispatcher(bool outrank)
{
    struct sched_param sp;

    sp.sched_priority = outrank ? DISPATCH_PRIORITY + 1 : PRODUCER_PRIORITY;
    (void)pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
}

/* -------------------------------------------------------------- the two timers */

/* A QNX timer delivers a sigevent rather than calling a function, and the sigevent
 * here is a pulse. So a timer expiry arrives through the same channel, at the same
 * priority, as every other event: there is no callback, no interrupt context and no
 * second path into the machine. That is the one place QNX's model is a better fit
 * than either of the others, and it is worth saying because the rest of this file is
 * a catalogue of the opposite. */
static int make_pulse_timer(timer_t *out, int code)
{
    struct sigevent sev;

    SIGEV_PULSE_INIT(&sev, connection_id, PULSE_PRIORITY, code, 0);
    return timer_create(CLOCK_MONOTONIC, &sev, out);
}

#define PULSE_CODE_HOLD   (_PULSE_CODE_MINAVAIL + 2)
#define PULSE_CODE_TICK   (_PULSE_CODE_MINAVAIL + 3)

static void arm_one_shot(timer_t t, uint32_t ms)
{
    struct itimerspec its;

    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = (time_t)(ms / 1000u);
    its.it_value.tv_nsec = (long)((ms % 1000u) * 1000000u);
    (void)timer_settime(t, 0, &its, NULL);
}

static void disarm(timer_t t)
{
    struct itimerspec its;

    memset(&its, 0, sizeof(its));
    (void)timer_settime(t, 0, &its, NULL);
}

/* ---------------------------------------------------------- the dispatch thread */

static bool ring_take(presence_event_t *out)
{
    bool took;

    (void)pthread_mutex_lock(&ring_lock);
    took = ring_count > 0u;
    if (took) {
        *out = ring[ring_head];
        ring_head = (ring_head + 1u) % PRESENCE_QUEUE_DEPTH;
        ring_count--;
    }
    (void)pthread_mutex_unlock(&ring_lock);
    return took;
}

/* The mirror, and the only state this adapter reads: hold_running false to true arms
 * the timer for the hold in force, true to false disarms it. Identical in intent to
 * the other two adapters and written in a third set of calls. */
static void mirror_hold_timer(bool was_running)
{
    if (!hold_timer_created) {
        return;
    }
    if (!was_running && ctx.hold_running) {
        arm_one_shot(hold_timer, ctx.settings.hold_ms);
    } else if (was_running && !ctx.hold_running) {
        disarm(hold_timer);
    }
}

static void *dispatch_entry(void *arg)
{
    (void)arg;

    for (;;) {
        struct _pulse pulse;
        presence_event_t ev;

        if (MsgReceivePulse(channel_id, &pulse, sizeof(pulse), NULL) == -1) {
            continue;
        }

        /* A hold or tick expiry is an event like any other and is appended to the same
         * ring, so that ONE queue holds everything and the order is one thing rather
         * than a race between a ring and a channel. */
        if (pulse.code == PULSE_CODE_HOLD || pulse.code == PULSE_CODE_TICK) {
            presence_event_t timer_ev;

            memset(&timer_ev, 0, sizeof(timer_ev));
            timer_ev.kind = (pulse.code == PULSE_CODE_HOLD) ? PRESENCE_EV_TIMEOUT
                                                            : PRESENCE_EV_TICK;
            timer_ev.at_ms = now_ms();

            (void)pthread_mutex_lock(&ring_lock);
            if (ring_count < PRESENCE_QUEUE_DEPTH) {
                ring[ring_tail] = timer_ev;
                ring_tail = (ring_tail + 1u) % PRESENCE_QUEUE_DEPTH;
                ring_count++;
            } else {
                dropped++;
            }
            (void)pthread_mutex_unlock(&ring_lock);
        }

        /* Drain. One pulse can stand for more than one event when two producers
         * appended between two receives, so taking only one per pulse would leave the
         * ring growing with no doorbell to come. */
        while (ring_take(&ev)) {
            bool was_running = ctx.hold_running;
            presence_result_t r = presence_dispatch(&ctx, &ev);

            if (r == PRESENCE_OK) {
                mirror_hold_timer(was_running);
                presence_indicators(ctx.state);
                if (trace != NULL) {
                    trace((uint8_t)ctx.last_row, &ctx);
                }
            }
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------ init */

bool presence_adapter_init(void)
{
    pthread_attr_t attr;
    struct sched_param sp;

    presence_init(&ctx, NULL);
    dropped = 0;
    ring_head = 0;
    ring_tail = 0;
    ring_count = 0;

    channel_id = ChannelCreate(0);
    if (channel_id == -1) {
        return false;
    }
    connection_id = ConnectAttach(0, 0, channel_id, _NTO_SIDE_CHANNEL, 0);
    if (connection_id == -1) {
        return false;
    }

    if (make_pulse_timer(&hold_timer, PULSE_CODE_HOLD) == -1) {
        return false;
    }
    hold_timer_created = true;

    if (make_pulse_timer(&tick_timer, PULSE_CODE_TICK) == -1) {
        return false;
    }
    tick_timer_created = true;

    /* Neither timer is armed here. The hold timer is armed by the table through the
     * mirror, and arming it now would start a hold for a room nobody has entered. */

    (void)pthread_attr_init(&attr);
    (void)pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    (void)pthread_attr_setschedpolicy(&attr, SCHED_RR);
    sp.sched_priority = DISPATCH_PRIORITY;
    (void)pthread_attr_setschedparam(&attr, &sp);

    if (pthread_create(&dispatch_tid, &attr, dispatch_entry, NULL) != 0) {
        return false;
    }
    return true;
}

bool presence_adapter_start_tick(uint32_t period_ms)
{
    struct itimerspec its;

    if (!tick_timer_created || period_ms == 0u) {
        return false;
    }
    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = (time_t)(period_ms / 1000u);
    its.it_value.tv_nsec = (long)((period_ms % 1000u) * 1000000u);
    its.it_interval = its.it_value;      /* periodic, unlike the hold timer */
    return timer_settime(tick_timer, 0, &its, NULL) == 0;
}
