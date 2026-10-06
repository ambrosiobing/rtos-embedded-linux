/* projects/02-the-claim/zephyr/claim_adapter.c: the same cascade under Zephyr.
 *
 * The second adapter, and the reason the contract in ../adapter/claim_adapter.h is written
 * to be satisfied rather than to describe one kernel. P01 learned that by having its first
 * adapter's header carry a FreeRTOS yield flag that Zephyr has no use for.
 *
 * EVERYTHING IS STATIC BY CONSTRUCTION. K_MSGQ_DEFINE and K_THREAD_STACK_DEFINE allocate at
 * build time, so where the FreeRTOS adapter sets configSUPPORT_DYNAMIC_ALLOCATION to 0 and
 * has the Makefile check the binary afterwards, there is nothing here to switch off.
 *
 * WHAT IS GENUINELY DIFFERENT FROM THE FREERTOS ADAPTER, in the two places it shows:
 *
 *   k_uptime_get_32 is already milliseconds. The FreeRTOS adapter multiplies a tick count
 *   by portTICK_PERIOD_MS to get the same thing.
 *
 *   k_msgq_put is the same call from a thread or an interrupt and wants no yield flag, so
 *   the interrupt post and the ordinary one differ only in the timestamp they read.
 *
 * AND THE ONE THAT COSTS A DESIGN DECISION. A k_timer expiry function runs in interrupt
 * context, so it may only post with K_NO_WAIT. FreeRTOS runs its timer callbacks in the
 * timer service task, which is less constrained and widens the window in which a cancel
 * races an expiry. Both are handled the same way here, by posting an event rather than
 * touching the inputs from the callback, which is what makes the dispatcher the only writer.
 */
#include <zephyr/kernel.h>

#include "claim_adapter.h"

#include <string.h>

/* ------------------------------------------------------------------ static storage */

K_MSGQ_DEFINE(event_queue, sizeof(claim_input_event_t), CLAIM_QUEUE_DEPTH, 4);

static struct k_timer grace_timer;

/* Zephyr counts priorities downwards: a smaller number preempts. FreeRTOS counts upwards.
 * That is the single most likely thing to be got backwards when porting between the two,
 * so the two adapters' priority constants are written to be read beside each other. */
#define DISPATCH_PRIORITY   K_PRIO_PREEMPT(5)
#define TEST_PRIORITY_BELOW K_PRIO_PREEMPT(6)
#define TEST_PRIORITY_ABOVE K_PRIO_PREEMPT(4)

#define DISPATCH_STACK_BYTES 2048

K_THREAD_STACK_DEFINE(dispatch_stack, DISPATCH_STACK_BYTES);
static struct k_thread dispatch_thread;

static claim_t ctx;
static claim_inputs_t inputs;
static claim_decision_t last;
static uint32_t dropped;
static claim_trace_fn trace;
static uint32_t grace_ms = 300u;

/* --------------------------------------------------------------------- the posting */

static bool post_stamped(const claim_input_event_t *ev)
{
    claim_input_event_t stamped = *ev;

    stamped.at_ms = k_uptime_get_32();

    /* K_NO_WAIT always, from a thread as well as from an interrupt. A full queue is a
     * refusal to be counted, never a reason for a producer to block behind the
     * dispatcher. */
    if (k_msgq_put(&event_queue, &stamped, K_NO_WAIT) != 0) {
        dropped++;
        return false;
    }
    return true;
}

bool claim_adapter_post(const claim_input_event_t *ev)
{
    return post_stamped(ev);
}

bool claim_adapter_post_from_isr(const claim_input_event_t *ev)
{
    /* The same call. Zephyr asks for no yield flag and needs no separate entry point, which
     * is precisely the difference that got a FreeRTOS out-parameter removed from a contract
     * three kernels have to satisfy. */
    return post_stamped(ev);
}

void claim_adapter_sleep_ms(uint32_t ms)
{
    k_msleep((int32_t)ms);
}

void claim_adapter_test_outrank_dispatcher(bool outrank)
{
    k_thread_priority_set(k_current_get(),
                          outrank ? TEST_PRIORITY_ABOVE : TEST_PRIORITY_BELOW);
}

uint32_t claim_adapter_events_dropped(void)
{
    return dropped;
}

const claim_t *claim_adapter_context(void)
{
    return &ctx;
}

claim_decision_t claim_adapter_last(void)
{
    return last;
}

void claim_adapter_set_trace(claim_trace_fn fn)
{
    trace = fn;
}

/* claim_indicators belongs to the application and is defined in main.c, as it is in the
 * FreeRTOS build. The adapter calls it and cannot tell a GPIO from a printf. */

/* ---------------------------------------------------------------- the grace timer */

static void grace_expired_cb(struct k_timer *t)
{
    claim_input_event_t ev;

    ARG_UNUSED(t);
    memset(&ev, 0, sizeof(ev));
    ev.kind = (uint8_t)CLAIM_EV_GRACE_EXPIRED;

    /* This runs in interrupt context, which is why it posts rather than writing the inputs:
     * the dispatcher stays the only writer and no lock is needed anywhere. */
    (void)post_stamped(&ev);
}

static void mirror_grace_timer(claim_code_t code)
{
    if (code == CLAIM_GRACE) {
        if (k_timer_remaining_get(&grace_timer) == 0u) {
            k_timer_start(&grace_timer, K_MSEC(grace_ms), K_NO_WAIT);
        }
    } else {
        k_timer_stop(&grace_timer);
    }
}

/* ------------------------------------------------------------------ the dispatcher */

static void apply(const claim_input_event_t *ev)
{
    switch ((claim_event_kind_t)ev->kind) {
    case CLAIM_EV_WINDOW:
        inputs.window_open = ev->u.flag;
        /* A new booking starts a new grace period and must not inherit the last one's
         * expiry, or the second booking of the day would no-show on arrival. */
        if (ev->u.flag) {
            inputs.past_grace = false;
        }
        break;
    case CLAIM_EV_PRESENCE:
        inputs.present = ev->u.flag;
        break;
    case CLAIM_EV_PRESS:
        inputs.long_press_pending = ev->u.flag;
        break;
    case CLAIM_EV_SERVICE:
        inputs.service = (claim_service_t)ev->u.svc.state;
        inputs.service_reason = (claim_svc_reason_t)ev->u.svc.reason;
        break;
    case CLAIM_EV_GRACE_EXPIRED:
        inputs.past_grace = true;
        break;
    case CLAIM_EV_ACTIVATE:
        ctx.settings.activated = ev->u.flag;
        break;
    case CLAIM_EV_SET_GRACE:
        grace_ms = ev->u.grace_ms;
        break;
    case CLAIM_EV_KIND_COUNT:
    default:
        break;
    }
}

static void dispatch_entry(void *a, void *b, void *c)
{
    claim_input_event_t ev;

    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    for (;;) {
        if (k_msgq_get(&event_queue, &ev, K_FOREVER) == 0) {
            apply(&ev);
            last = claim_step(&ctx, &inputs, ev.at_ms / 1000u);
            mirror_grace_timer(last.code);
            claim_indicators(last.code);
            if (trace != NULL) {
                trace(&last, &ctx);
            }
        }
    }
}

/* ------------------------------------------------------------------------- the init */

bool claim_adapter_init(void)
{
    claim_init(&ctx);
    memset(&inputs, 0, sizeof(inputs));
    inputs.service = SVC_OK;
    inputs.service_reason = SVC_REASON_NONE;
    last.code = CLAIM_INVISIBLE;
    last.arm = 0u;
    dropped = 0u;
    trace = NULL;

    k_timer_init(&grace_timer, grace_expired_cb, NULL);

    (void)k_thread_create(&dispatch_thread, dispatch_stack,
                          K_THREAD_STACK_SIZEOF(dispatch_stack),
                          dispatch_entry, NULL, NULL, NULL,
                          DISPATCH_PRIORITY, 0, K_NO_WAIT);

    /* The queue, the stack and the thread object all exist before this function runs, so
     * there is nothing here that can fail for want of memory. The return value is in the
     * contract because FreeRTOS can fail here and a contract written to one kernel's
     * abilities is the mistake this header exists to avoid. */
    return true;
}
