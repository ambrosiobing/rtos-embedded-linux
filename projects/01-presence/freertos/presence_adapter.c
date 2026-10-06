/* projects/01-presence/freertos/presence_adapter.c: a thread, a queue, two timers.
 *
 * Nothing in this file decides anything. The header says why in full; the short
 * version is that a threshold found here means this file is wrong, not the table.
 *
 * Everything is statically allocated. configSUPPORT_DYNAMIC_ALLOCATION is 0 in
 * FreeRTOSConfig.h, so an accidental call to a create function that allocates would
 * not link, which is a stronger guarantee than reading a map file and is why the
 * config is written that way round.
 */
#include "presence_adapter.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include <string.h>

/* ------------------------------------------------------- the static objects */

/* The queue is PRESENCE_QUEUE_DEPTH events of sizeof(presence_event_t), which is
 * 16 times 20 bytes on both the host and the board. Both numbers come from
 * presence.h so that the memory budget in docs/DESIGN.md is one multiplication and
 * cannot drift from the code. */
static StaticQueue_t queue_control;
static uint8_t queue_storage[PRESENCE_QUEUE_DEPTH * sizeof(presence_event_t)];
static QueueHandle_t queue;

static StaticTimer_t hold_control;
static TimerHandle_t hold_timer;

static StaticTimer_t tick_control;
static TimerHandle_t tick_timer;

#define DISPATCH_STACK_WORDS 512u
static StaticTask_t dispatch_control;
static StackType_t dispatch_stack[DISPATCH_STACK_WORDS];

/* The machine. One node, one context, and the dispatch task is the only writer. */
static presence_t ctx;
static uint32_t dropped;
static presence_trace_fn trace;

/* The tick period for the heartbeat. Not a decision about presence: the table has a
 * TICK row in every state and all four do nothing, so this exists to prove the
 * dispatcher keeps running and to give the indicators a refresh point. */
#define TICK_PERIOD_MS 1000u

/* ------------------------------------------------------------------ the clock */

/* One clock for every event, which is requirement 2. A reading stamped from one
 * source and a timeout from another would make the guard on row 17 compare two
 * different time bases, and the guard is the only thing standing between a stale
 * expiry and a release thirty seconds early. */
static uint32_t now_ms(void)
{
    return (uint32_t)((uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS);
}

static uint32_t now_ms_from_isr(void)
{
    return (uint32_t)((uint64_t)xTaskGetTickCountFromISR() *
                      (uint64_t)portTICK_PERIOD_MS);
}

/* --------------------------------------------------------------- the posting */

bool presence_adapter_post(const presence_event_t *ev)
{
    presence_event_t stamped;

    if (ev == NULL || queue == NULL) {
        return false;
    }
    stamped = *ev;
    stamped.at_ms = now_ms();

    /* To the BACK, with no wait. The order is the guarantee and the wait would be a
     * second way for a producer to be blocked by the dispatcher. */
    if (xQueueSendToBack(queue, &stamped, 0) != pdPASS) {
        dropped++;
        return false;
    }
    return true;
}

bool presence_adapter_post_from_isr(const presence_event_t *ev,
                                    long *higher_priority_task_woken)
{
    presence_event_t stamped;
    BaseType_t woken = pdFALSE;

    if (ev == NULL || queue == NULL) {
        return false;
    }
    stamped = *ev;
    stamped.at_ms = now_ms_from_isr();

    if (xQueueSendToBackFromISR(queue, &stamped, &woken) != pdPASS) {
        dropped++;
        if (higher_priority_task_woken != NULL) {
            *higher_priority_task_woken = (long)woken;
        }
        return false;
    }
    if (higher_priority_task_woken != NULL) {
        *higher_priority_task_woken = (long)woken;
    }
    return true;
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

/* ---------------------------------------------------------- the timer callbacks */

/* Both of these post an event and do nothing else. A timer callback that looked at
 * the state and decided whether the expiry still mattered would be the generation
 * counter docs/RTOS_VARIANTS.md rejects, and the decision would have left the
 * table. */
static void hold_expired_cb(TimerHandle_t t)
{
    presence_event_t ev;

    (void)t;
    memset(&ev, 0, sizeof(ev));
    ev.kind = PRESENCE_EV_TIMEOUT;
    (void)presence_adapter_post(&ev);
}

static void tick_cb(TimerHandle_t t)
{
    presence_event_t ev;

    (void)t;
    memset(&ev, 0, sizeof(ev));
    ev.kind = PRESENCE_EV_TICK;
    (void)presence_adapter_post(&ev);
}

/* ------------------------------------------------------------ the dispatch task */

/* The mirror, and the only state this adapter reads. hold_running going false to
 * true arms the kernel timer for the hold currently in force; true to false cancels
 * it. The period is read from the settings rather than from a constant here,
 * because a SETTINGS event can change it and the table is where that is decided.
 *
 * xTimerChangePeriod also starts the timer, which is what is wanted on the arming
 * edge: one call rather than a change and a start that could be interleaved. */
static void mirror_hold_timer(bool was_running)
{
    if (!was_running && ctx.hold_running) {
        (void)xTimerChangePeriod(hold_timer, pdMS_TO_TICKS(ctx.settings.hold_ms), 0);
    } else if (was_running && !ctx.hold_running) {
        (void)xTimerStop(hold_timer, 0);
    }
}

static void dispatch_task(void *arg)
{
    (void)arg;

    for (;;) {
        presence_event_t ev;

        if (xQueueReceive(queue, &ev, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        bool was_running = ctx.hold_running;
        presence_result_t r = presence_dispatch(&ctx, &ev);

        if (r == PRESENCE_OK) {
            mirror_hold_timer(was_running);
            presence_indicators(ctx.state);
            if (trace != NULL) {
                trace((uint8_t)ctx.last_row, &ctx);
            }
        }
        /* A result other than OK means the table had no row, which is a defect in
         * the table and not an input to tolerate. There is deliberately nothing
         * here that recovers from it: the host test asserts it never happens, and
         * on the board the fault row is how an untrustworthy reading is reported. */
    }
}

/* ------------------------------------------------------------------------ init */

bool presence_adapter_init(void)
{
    presence_init(&ctx, NULL);
    dropped = 0;

    queue = xQueueCreateStatic(PRESENCE_QUEUE_DEPTH, sizeof(presence_event_t),
                               queue_storage, &queue_control);
    if (queue == NULL) {
        return false;
    }

    /* One-shot: the reload is pdFALSE, because a hold expires once and the table
     * decides what happens next. A reloading hold timer would post a TIMEOUT every
     * hold_ms for ever, and rows 4 and 11 would quietly absorb all of them. */
    hold_timer = xTimerCreateStatic("hold", pdMS_TO_TICKS(PRESENCE_DEFAULT_HOLD_MS),
                                    pdFALSE, NULL, hold_expired_cb, &hold_control);
    if (hold_timer == NULL) {
        return false;
    }

    tick_timer = xTimerCreateStatic("tick", pdMS_TO_TICKS(TICK_PERIOD_MS),
                                    pdTRUE, NULL, tick_cb, &tick_control);
    if (tick_timer == NULL) {
        return false;
    }

    if (xTaskCreateStatic(dispatch_task, "presence", DISPATCH_STACK_WORDS, NULL,
                          tskIDLE_PRIORITY + 2, dispatch_stack,
                          &dispatch_control) == NULL) {
        return false;
    }

    /* Neither timer is started here. The hold timer is armed by the table, through
     * the mirror, and starting it now would arm a hold for a room nobody has
     * entered. The heartbeat is started by its own call, for the reason in the
     * header: a scripted test needs a quiet queue. */
    return true;
}

bool presence_adapter_start_tick(uint32_t period_ms)
{
    if (tick_timer == NULL || period_ms == 0u) {
        return false;
    }
    return xTimerChangePeriod(tick_timer, pdMS_TO_TICKS(period_ms), 0) == pdPASS;
}
