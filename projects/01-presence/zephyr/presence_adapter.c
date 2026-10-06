/* projects/01-presence/zephyr/presence_adapter.c: the same contract, under Zephyr.
 *
 * ../adapter/presence_adapter.h is the whole of what this file implements, and
 * ../freertos/presence_adapter.c implements exactly the same thing. Reading the two
 * side by side is the point of having both: the table, the three requirements and the
 * four test phases are identical, and what differs is only which kernel call does
 * which job.
 *
 * Everything is statically allocated, as under FreeRTOS, but by a different route.
 * FreeRTOS needed configSUPPORT_DYNAMIC_ALLOCATION at 0 so that xQueueCreate would
 * not link; Zephyr's K_MSGQ_DEFINE and K_THREAD_STACK_DEFINE are static by
 * construction and there is nothing to switch off. CONFIG_HEAP_MEM_POOL_SIZE is 0 in
 * prj.conf so that k_malloc is not there either.
 *
 * WHAT IS GENUINELY SIMPLER HERE, and it is worth naming because most of the
 * differences are cosmetic:
 *
 *   * k_uptime_get_32 returns milliseconds. The FreeRTOS adapter multiplies a tick
 *     count by portTICK_PERIOD_MS in 64 bits to avoid an overflow nobody would see
 *     until the node had been up for weeks. Here there is nothing to get wrong.
 *   * Posting from an interrupt is the same call as posting from a thread, and there
 *     is no yield flag to hand back. That is the difference that moved the out
 *     parameter out of the shared header.
 *   * K_MSEC takes milliseconds, so a hold period is written as the settings hold it.
 *
 * AND WHAT IS NOT. A k_timer expiry function runs in interrupt context, so it may
 * only post with K_NO_WAIT and must treat a full queue as an event lost rather than
 * as something to wait for. FreeRTOS runs its timer callbacks in the timer service
 * task, which makes its cancel window wider but its callbacks less constrained. The
 * table is unaffected either way: rows 4, 11 and 18 exist because NEITHER kernel can
 * un-queue an expiry that has already fired.
 */
#include "presence_adapter.h"

#include <zephyr/kernel.h>

#include <string.h>

/* ------------------------------------------------------- the static objects */

/* PRESENCE_QUEUE_DEPTH events of sizeof(presence_event_t), both from presence.h, so
 * the memory budget in docs/DESIGN.md is one multiplication and cannot drift. The
 * alignment is four, which is what the event's uint32_t members need. */
K_MSGQ_DEFINE(event_queue, sizeof(presence_event_t), PRESENCE_QUEUE_DEPTH, 4);

static struct k_timer hold_timer;
static struct k_timer tick_timer;

/* Zephyr counts priorities downwards: a smaller number preempts a larger one. The
 * dispatch thread therefore sits one number BELOW the producer, which is the opposite
 * spelling of the same arrangement FreeRTOS uses and the same arrangement in effect.
 * A post is taken before the producer's next statement, so the scripted phases are
 * deterministic rather than a race that usually passes. */
#define DISPATCH_PRIORITY 5
#define PRODUCER_PRIORITY 6

#define DISPATCH_STACK_BYTES 2048
K_THREAD_STACK_DEFINE(dispatch_stack, DISPATCH_STACK_BYTES);
static struct k_thread dispatch_thread;

/* The machine. One node, one context, and the dispatch thread is the only writer. */
static presence_t ctx;
static uint32_t dropped;
static presence_trace_fn trace;

#define TICK_PERIOD_MS 1000u

/* --------------------------------------------------------------- the posting */

/* One clock for every event, which is requirement 2 of the contract. A reading
 * stamped from one source and a timeout from another would make the guard on row 17
 * compare two different time bases. k_uptime_get_32 is already milliseconds and is
 * safe from an interrupt, so both posts use it and there is one clock. */
static bool post_stamped(const presence_event_t *ev)
{
    presence_event_t stamped;

    if (ev == NULL) {
        return false;
    }
    stamped = *ev;
    stamped.at_ms = k_uptime_get_32();

    /* K_NO_WAIT always: the order is the guarantee and a wait would be a second way
     * for a producer to be blocked by the dispatcher. k_msgq_put appends, and
     * k_msgq_put_front exists and is never called here, because it would silently
     * remove the ordering that makes a stale expiry harmless. */
    if (k_msgq_put(&event_queue, &stamped, K_NO_WAIT) != 0) {
        dropped++;
        return false;
    }
    return true;
}

bool presence_adapter_post(const presence_event_t *ev)
{
    return post_stamped(ev);
}

/* The same call, and no yield flag. Under FreeRTOS this needs
 * xQueueSendToBackFromISR and a portYIELD_FROM_ISR; Zephyr reschedules on its own.
 * That asymmetry is the reason the shared header has no out parameter. */
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
    k_msleep((int32_t)ms);
}

void presence_adapter_test_outrank_dispatcher(bool outrank)
{
    k_thread_priority_set(k_current_get(),
                          outrank ? DISPATCH_PRIORITY - 1 : PRODUCER_PRIORITY);
}

/* ---------------------------------------------------------- the timer callbacks */

/* Both run in interrupt context, post one event, and do nothing else. A callback that
 * looked at the state and decided whether the expiry still mattered would be the
 * generation counter docs/RTOS_VARIANTS.md rejects, and the decision would have left
 * the table. */
static void hold_expired_cb(struct k_timer *t)
{
    presence_event_t ev;

    ARG_UNUSED(t);
    memset(&ev, 0, sizeof(ev));
    ev.kind = PRESENCE_EV_TIMEOUT;
    (void)post_stamped(&ev);
}

static void tick_cb(struct k_timer *t)
{
    presence_event_t ev;

    ARG_UNUSED(t);
    memset(&ev, 0, sizeof(ev));
    ev.kind = PRESENCE_EV_TICK;
    (void)post_stamped(&ev);
}

/* ---------------------------------------------------------- the dispatch thread */

/* The mirror, and the only state this adapter reads. hold_running going false to true
 * arms the kernel timer for the hold in force; true to false cancels it. The period
 * comes from the settings rather than from a constant, because a SETTINGS event can
 * change it and the table is where that is decided.
 *
 * One shot: the period argument is the duration and the second is K_NO_WAIT, so it
 * does not reload. A reloading hold timer would post a TIMEOUT every hold_ms for
 * ever and rows 4, 11 and 18 would quietly absorb all of them. */
static void mirror_hold_timer(bool was_running)
{
    if (!was_running && ctx.hold_running) {
        k_timer_start(&hold_timer, K_MSEC(ctx.settings.hold_ms), K_NO_WAIT);
    } else if (was_running && !ctx.hold_running) {
        k_timer_stop(&hold_timer);
    }
}

static void dispatch_entry(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    for (;;) {
        presence_event_t ev;
        bool was_running;
        presence_result_t r;

        if (k_msgq_get(&event_queue, &ev, K_FOREVER) != 0) {
            continue;
        }

        was_running = ctx.hold_running;
        r = presence_dispatch(&ctx, &ev);

        if (r == PRESENCE_OK) {
            mirror_hold_timer(was_running);
            presence_indicators(ctx.state);
            if (trace != NULL) {
                trace((uint8_t)ctx.last_row, &ctx);
            }
        }
        /* A result other than OK means the table had no row, which is a defect in the
         * table and not an input to tolerate. Nothing here recovers from it: the
         * phases assert it never happens. */
    }
}

/* ------------------------------------------------------------------------ init */

bool presence_adapter_init(void)
{
    presence_init(&ctx, NULL);
    dropped = 0;

    k_timer_init(&hold_timer, hold_expired_cb, NULL);
    k_timer_init(&tick_timer, tick_cb, NULL);

    /* Neither timer is started here. The hold timer is armed by the table through the
     * mirror, and starting it now would arm a hold for a room nobody has entered. The
     * heartbeat has its own call, so a scripted phase can run against a quiet queue. */

    (void)k_thread_create(&dispatch_thread, dispatch_stack,
                          K_THREAD_STACK_SIZEOF(dispatch_stack),
                          dispatch_entry, NULL, NULL, NULL,
                          DISPATCH_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&dispatch_thread, "presence");

    return true;
}

bool presence_adapter_start_tick(uint32_t period_ms)
{
    if (period_ms == 0u) {
        return false;
    }
    k_timer_start(&tick_timer, K_MSEC(period_ms), K_MSEC(period_ms));
    return true;
}
