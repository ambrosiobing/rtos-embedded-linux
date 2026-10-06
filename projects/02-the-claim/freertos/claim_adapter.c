/* projects/02-the-claim/freertos/claim_adapter.c: the cascade under FreeRTOS.
 *
 * This file is the only one in the project that knows FreeRTOS exists. ../c/claim.c knows
 * no kernel, and ../adapter/phases.c includes no kernel header at all.
 *
 * EVERYTHING IS STATIC. configSUPPORT_DYNAMIC_ALLOCATION is 0, so an allocating create
 * does not fail at run time, it fails to link, and the Makefile checks the binary for
 * pvPortMalloc afterwards. A queue, a task and a timer are all declared here as storage
 * this file owns.
 *
 * THE ONE THING THIS ADAPTER DOES THAT P01'S DOES NOT. P01's machine consumes an event and
 * advances a state, so its adapter forwards events. This one's cascade is a pure function
 * of a set of inputs, so the adapter holds the inputs, an event changes one of them, and
 * the decision is recomputed from the whole set. That is "a claim is a decision, not a
 * state" arriving at a kernel, and it is why there is no event queue inside claim.c.
 *
 * THE GRACE TIMER IS THE INTERESTING PART. When the cascade says GRACE the adapter arms a
 * one-shot timer; when it says anything else the adapter stops it. If the timer fires, it
 * posts GRACE_EXPIRED, the dispatcher sets past_grace, and the next decision is a no-show
 * produced by a clock rather than by a test. Opening a window clears past_grace, because a
 * new booking starts a new grace period and must not inherit the last one's expiry.
 */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include "claim_adapter.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ static storage */

static StaticQueue_t queue_control;
static uint8_t queue_storage[CLAIM_QUEUE_DEPTH * sizeof(claim_input_event_t)];
static QueueHandle_t queue;

static StaticTimer_t grace_control;
static TimerHandle_t grace_timer;

#define DISPATCH_STACK_WORDS (configMINIMAL_STACK_SIZE * 2)
#define DISPATCH_PRIORITY    (tskIDLE_PRIORITY + 2)
#define TEST_PRIORITY_BELOW  (tskIDLE_PRIORITY + 1)
#define TEST_PRIORITY_ABOVE  (tskIDLE_PRIORITY + 3)

static StaticTask_t dispatch_control;
static StackType_t dispatch_stack[DISPATCH_STACK_WORDS];

static claim_t ctx;
static claim_inputs_t inputs;
static claim_decision_t last;
static uint32_t dropped;
static claim_trace_fn trace;
static uint32_t grace_ms = 300u;   /* the test shortens this; the chapter's default is 300 s */

/* ---------------------------------------------------------------------- the clock */

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/* --------------------------------------------------------------------- the posting */

bool claim_adapter_post(const claim_input_event_t *ev)
{
    claim_input_event_t copy = *ev;

    copy.at_ms = now_ms();
    if (xQueueSendToBack(queue, &copy, 0) != pdPASS) {
        dropped++;
        return false;
    }
    return true;
}

bool claim_adapter_post_from_isr(const claim_input_event_t *ev)
{
    BaseType_t woken = pdFALSE;
    claim_input_event_t copy = *ev;

    /* No out-parameter in the contract: this adapter yields on its own behalf, because
     * asking every kernel for a yield flag was one kernel's calling convention. */
    copy.at_ms = (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    if (xQueueSendToBackFromISR(queue, &copy, &woken) != pdPASS) {
        dropped++;
        return false;
    }
    portYIELD_FROM_ISR(woken);
    return true;
}

void claim_adapter_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void claim_adapter_test_outrank_dispatcher(bool outrank)
{
    vTaskPrioritySet(NULL, outrank ? TEST_PRIORITY_ABOVE : TEST_PRIORITY_BELOW);
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

/* claim_indicators is NOT defined here. The lamps belong to the application, which is why
 * the contract declares the hook and host_main.c supplies it: the adapter calls it and
 * cannot tell a GPIO from a printf. Defining it here as well would be a duplicate symbol,
 * which is how P01's shape makes the ownership obvious rather than conventional. */

/* ---------------------------------------------------------------- the grace timer */

static void grace_expired_cb(TimerHandle_t t)
{
    claim_input_event_t ev;

    (void)t;
    memset(&ev, 0, sizeof(ev));
    ev.kind = (uint8_t)CLAIM_EV_GRACE_EXPIRED;

    /* The timer service task is a task, so the ordinary post is correct here. */
    (void)claim_adapter_post(&ev);
}

static void mirror_grace_timer(claim_code_t code)
{
    if (code == CLAIM_GRACE) {
        if (xTimerIsTimerActive(grace_timer) == pdFALSE) {
            (void)xTimerChangePeriod(grace_timer, pdMS_TO_TICKS(grace_ms), 0);
            (void)xTimerStart(grace_timer, 0);
        }
    } else if (xTimerIsTimerActive(grace_timer) != pdFALSE) {
        (void)xTimerStop(grace_timer, 0);
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

static void dispatch_task(void *arg)
{
    claim_input_event_t ev;

    (void)arg;
    for (;;) {
        if (xQueueReceive(queue, &ev, portMAX_DELAY) == pdPASS) {
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
    TaskHandle_t dispatch;

    claim_init(&ctx);
    memset(&inputs, 0, sizeof(inputs));
    inputs.service = SVC_OK;
    inputs.service_reason = SVC_REASON_NONE;
    last.code = CLAIM_INVISIBLE;
    last.arm = 0u;
    dropped = 0u;
    trace = NULL;

    queue = xQueueCreateStatic(CLAIM_QUEUE_DEPTH, sizeof(claim_input_event_t),
                               queue_storage, &queue_control);
    if (queue == NULL) {
        return false;
    }

    grace_timer = xTimerCreateStatic("grace", pdMS_TO_TICKS(grace_ms), pdFALSE, NULL,
                                     grace_expired_cb, &grace_control);
    if (grace_timer == NULL) {
        return false;
    }

    dispatch = xTaskCreateStatic(dispatch_task, "dispatch", DISPATCH_STACK_WORDS, NULL,
                                 DISPATCH_PRIORITY, dispatch_stack, &dispatch_control);
    return dispatch != NULL;
}
