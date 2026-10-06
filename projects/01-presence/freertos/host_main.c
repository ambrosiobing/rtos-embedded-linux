/* projects/01-presence/freertos/host_main.c: the adapter, exercised for real.
 *
 * WHAT THIS PROVES THAT ../c/test_presence.c DOES NOT. That file calls
 * presence_dispatch directly, which is the right way to test a table and says
 * nothing about a kernel. This one posts the same events into a real FreeRTOS queue,
 * lets a real dispatch task take them, and lets a real software timer expire, then
 * asserts that the row sequence is identical. Same rows, different plumbing, which
 * is the whole claim an adapter makes.
 *
 * FOUR PHASES, and the second and third are the ones worth having.
 *
 *   1. The scripted sequence through the queue. The dispatch task runs at a higher
 *      priority than this one, so each post is consumed before the next is made and
 *      the comparison is deterministic rather than timing-dependent.
 *   2. A release by the kernel timer rather than by an injected event. The hold is
 *      shortened to 60 ms through a SETTINGS event, which is the table's own way of
 *      changing it, and then nothing is posted at all: the timer fires, the adapter
 *      posts the TIMEOUT, and row 17 releases. Until this runs, every TIMEOUT in
 *      this project has been written by a test.
 *   3. A full queue, which is the one failure this design cannot tolerate and cannot
 *      detect afterwards. This phase raises its own priority above the dispatcher so
 *      that nothing drains, posts one event more than the queue holds, and asserts
 *      that the refusal was counted. An adapter that dropped silently would pass
 *      every other test in this directory.
 *   4. The heartbeat, started last for the reason in presence_adapter.h.
 *
 * It exits non-zero on the first failure, so CI and a human read the same thing.
 */
#include "presence_adapter.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                                      \
    do {                                                                      \
        if (!(cond)) {                                                        \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__);                     \
            printf(__VA_ARGS__);                                              \
            printf("\n");                                                     \
            failures++;                                                       \
        }                                                                     \
    } while (0)

/* ------------------------------------------------- what the application supplies */

/* The lamps. On the board these are three GPIOs; here the state is printed, and the
 * adapter cannot tell the difference, which is the point of the hook. */
void presence_indicators(presence_state_t state)
{
    (void)state;
}

void presence_freertos_assert(const char *file, unsigned long line)
{
    printf("  FAIL kernel assertion at %s:%lu\n", file, line);
    fflush(stdout);
    exit(2);
}

/* configSUPPORT_DYNAMIC_ALLOCATION is 0, so FreeRTOS cannot allocate the memory for
 * its own idle and timer tasks and asks the application for it. This is the cost of
 * making an accidental dynamic create a link error, and it is worth paying. */
static StaticTask_t idle_control;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

/* The third parameter is configSTACK_DEPTH_TYPE and not uint32_t. It defaults to
 * StackType_t, which is an unsigned long on this host and so a different width, and
 * the first build said so: "conflicting types, have void(..., uint32_t *), previous
 * declaration void(..., StackType_t *)". Written as the kernel spells it, so that a
 * port where StackType_t is 16 bits does not quietly disagree. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxTaskTCB,
                                   StackType_t **ppxTaskStack,
                                   configSTACK_DEPTH_TYPE *puxTaskStackSize)
{
    *ppxTaskTCB = &idle_control;
    *ppxTaskStack = idle_stack;
    *puxTaskStackSize = (configSTACK_DEPTH_TYPE)configMINIMAL_STACK_SIZE;
}

static StaticTask_t timer_control;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB,
                                    StackType_t **ppxTimerStack,
                                    configSTACK_DEPTH_TYPE *puxTimerStackSize)
{
    *ppxTimerTCB = &timer_control;
    *ppxTimerStack = timer_stack;
    *puxTimerStackSize = (configSTACK_DEPTH_TYPE)configTIMER_TASK_STACK_DEPTH;
}

/* ------------------------------------------------------------------- the trace */

#define TRACE_MAX 64u
static uint8_t seen[TRACE_MAX];
static unsigned seen_count;

static void record(uint8_t row, const presence_t *after)
{
    CHECK(presence_check_invariants(after),
          "the invariant broke after row %u", (unsigned)row);
    if (seen_count < TRACE_MAX) {
        seen[seen_count] = row;
    }
    seen_count++;
}

static void expect_rows(const uint8_t *want, unsigned n, const char *what)
{
    CHECK(seen_count == n, "%s: %u rows taken, expected %u", what, seen_count, n);
    for (unsigned i = 0; i < n && i < seen_count; i++) {
        CHECK(seen[i] == want[i], "%s: row %u of the sequence was %u, expected %u",
              what, i, (unsigned)seen[i], (unsigned)want[i]);
    }
}

/* ------------------------------------------------------------------ the posting */

#define NEAR 1000u
#define FAR  9000u

static void post_reading(uint16_t mm)
{
    presence_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.kind = PRESENCE_EV_READING;
    ev.u.range_mm = mm;
    CHECK(presence_adapter_post(&ev), "a reading was refused by the queue");
}

static void post_settings(uint16_t runs, uint32_t hold_ms, uint16_t max_mm)
{
    presence_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.kind = PRESENCE_EV_SETTINGS;
    ev.u.settings.arrive_runs = runs;
    ev.u.settings.hold_ms = hold_ms;
    ev.u.settings.range_mm_max = max_mm;
    CHECK(presence_adapter_post(&ev), "a settings change was refused by the queue");
}

/* Let the dispatcher run. The dispatch task is at a higher priority, so a single
 * yield is enough for everything already queued; the delay is for the phases that
 * wait on a timer rather than on the queue. */
static void settle(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* -------------------------------------------------------------------- the phases */

static void phase_the_scripted_sequence(void)
{
    static const uint8_t want[] = { 1, 0, 8, 9, 15, 9 };

    printf("1. the C test's sequence, through a real queue and a real task\n");
    seen_count = 0;

    post_reading(NEAR);          /* row 1, counting towards the arrival */
    post_reading(NEAR);          /* row 0, arrived */
    post_reading(NEAR);          /* row 8, refresh */
    post_reading(FAR);           /* row 9, hold starts, kernel timer armed */
    post_reading(NEAR);          /* row 15, hold cancelled, kernel timer stopped */
    post_reading(FAR);           /* row 9 again, a second hold */
    settle(20);

    expect_rows(want, (unsigned)(sizeof(want) / sizeof(want[0])), "the sequence");
    CHECK(presence_adapter_context()->state == PRESENCE_HELD,
          "the room should be held at the end of the sequence");
    CHECK(presence_adapter_context()->releases == 0u,
          "nothing has been released yet");
}

static void phase_the_timer_releases(void)
{
    printf("2. the hold expires by the kernel timer, not by an injected event\n");
    seen_count = 0;

    /* Shorten the hold through the table, which is where that decision lives. The
     * settings row does not disturb the hold that is already running, so the timer
     * armed at the end of phase 1 is still the thirty second one; the next hold is
     * the short one. */
    post_settings(2, 60, 2500);
    settle(20);
    post_reading(NEAR);          /* cancel the long hold, back to occupied */
    post_reading(FAR);           /* a new hold, 60 ms this time */
    settle(20);
    CHECK(presence_adapter_context()->state == PRESENCE_HELD, "held, briefly");

    /* Nothing is posted now. If row 17 is reached it is because a software timer
     * fired, the callback posted a TIMEOUT, and the dispatcher took it. */
    settle(300);
    CHECK(presence_adapter_context()->state == PRESENCE_FREE,
          "the kernel timer should have released the hold");
    CHECK(presence_adapter_context()->releases == 1u,
          "exactly one release, by the timer");
    CHECK(seen_count >= 1u && seen[seen_count - 1u] == 17u,
          "the last row should be 17, the release");
}

static void phase_a_full_queue_is_counted(void)
{
    unsigned refused = 0;

    printf("3. a full queue loses an event, and says so\n");
    CHECK(presence_adapter_events_dropped() == 0u,
          "nothing should have been dropped before this phase");

    /* Above the dispatcher, so nothing drains while this runs. Without this the
     * queue never fills and the phase would pass by never testing anything. */
    vTaskPrioritySet(NULL, tskIDLE_PRIORITY + 3);

    for (unsigned i = 0; i < PRESENCE_QUEUE_DEPTH + 2u; i++) {
        presence_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.kind = PRESENCE_EV_TICK;
        if (!presence_adapter_post(&ev)) {
            refused++;
        }
    }

    vTaskPrioritySet(NULL, tskIDLE_PRIORITY + 1);
    settle(50);

    CHECK(refused == 2u, "two posts past the end should be refused, %u were", refused);
    CHECK(presence_adapter_events_dropped() == refused,
          "the adapter's count is %u and the refusals were %u",
          (unsigned)presence_adapter_events_dropped(), refused);
}

static void phase_the_heartbeat(void)
{
    uint32_t before;

    printf("4. the heartbeat, started last so the phases above stay quiet\n");
    before = presence_adapter_context()->row_taken[3];   /* free, tick */
    CHECK(presence_adapter_start_tick(20u), "the heartbeat should start");
    settle(120);
    CHECK(presence_adapter_context()->row_taken[3] > before,
          "a tick should have arrived in free");
}

/* ---------------------------------------------------------------------- the task */

static void test_task(void *arg)
{
    (void)arg;

    presence_adapter_set_trace(record);

    phase_the_scripted_sequence();
    phase_the_timer_releases();
    phase_a_full_queue_is_counted();
    phase_the_heartbeat();

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    fflush(stdout);
    exit(failures ? 1 : 0);
}

#define TEST_STACK_WORDS ( configMINIMAL_STACK_SIZE )
static StaticTask_t test_control;
static StackType_t test_stack[TEST_STACK_WORDS];

int main(void)
{
    printf("the presence table under FreeRTOS, on a host\n\n");

    if (!presence_adapter_init()) {
        printf("  FAIL the adapter could not create its static objects\n");
        return 1;
    }

    if (xTaskCreateStatic(test_task, "test", TEST_STACK_WORDS, NULL,
                          tskIDLE_PRIORITY + 1, test_stack,
                          &test_control) == NULL) {
        printf("  FAIL the test task could not be created\n");
        return 1;
    }

    vTaskStartScheduler();

    /* Only reached if the scheduler returns, which it should not. */
    printf("  FAIL the scheduler returned\n");
    return 1;
}
