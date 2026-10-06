/* projects/01-presence/adapter/phases.c: four phases, no kernel header.
 *
 * WHAT THIS PROVES THAT ../c/test_presence.c DOES NOT. That file calls
 * presence_dispatch directly, which is the right way to test a table and says nothing
 * about a kernel. This one posts the same events into a real queue, lets a real
 * dispatch thread take them, and lets a real timer expire, then asserts the row
 * sequence is identical. Same rows, different plumbing.
 *
 * AND IT INCLUDES NO KERNEL HEADER. Everything it needs beyond the adapter contract
 * is two calls, presence_adapter_sleep_ms and presence_adapter_test_outrank_dispatcher,
 * which each adapter supplies. That is what lets one test run against every kernel
 * rather than each kernel having a test of its own that nobody can compare.
 *
 * THE FOUR, and the second and third are the ones worth having.
 *
 *   1. The scripted sequence through the queue. The dispatch thread runs at a higher
 *      priority than this one, so each post is consumed before the next is made and
 *      the comparison is deterministic rather than timing-dependent.
 *   2. A release by the kernel timer rather than by an injected event. The hold is
 *      shortened to 60 ms through a SETTINGS event, which is the table's own way of
 *      changing it, and then nothing is posted at all: the timer fires, the adapter
 *      posts the TIMEOUT, and row 17 releases. Every other TIMEOUT in this project
 *      was written by a test.
 *   3. A full queue, which is the one failure this design cannot tolerate and cannot
 *      detect afterwards. This phase outranks the dispatcher so that nothing drains,
 *      posts more than the queue holds, and asserts the refusals were counted. An
 *      adapter that dropped silently would pass every other phase here.
 *   4. The heartbeat, started last so the phases above stay quiet.
 */
#include "phases.h"

#include "presence_adapter.h"

#include <stdio.h>
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

/* -------------------------------------------------------------------- the phases */

static void phase_the_scripted_sequence(void)
{
    static const uint8_t want[] = { 1, 0, 8, 9, 15, 9 };

    printf("1. the C test's sequence, through a real queue and a real thread\n");
    seen_count = 0;

    post_reading(NEAR);          /* row 1, counting towards the arrival */
    post_reading(NEAR);          /* row 0, arrived */
    post_reading(NEAR);          /* row 8, refresh */
    post_reading(FAR);           /* row 9, hold starts, kernel timer armed */
    post_reading(NEAR);          /* row 15, hold cancelled, kernel timer stopped */
    post_reading(FAR);           /* row 9 again, a second hold */
    presence_adapter_sleep_ms(20);

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
     * settings row does not disturb a hold already running, so the timer armed at the
     * end of phase 1 is still the thirty second one; the next hold is the short one. */
    post_settings(2, 60, 2500);
    presence_adapter_sleep_ms(20);
    post_reading(NEAR);          /* cancel the long hold, back to occupied */
    post_reading(FAR);           /* a new hold, 60 ms this time */
    presence_adapter_sleep_ms(20);
    CHECK(presence_adapter_context()->state == PRESENCE_HELD, "held, briefly");

    /* Nothing is posted now. If row 17 is reached it is because a kernel timer fired,
     * the callback posted a TIMEOUT, and the dispatch thread took it. */
    presence_adapter_sleep_ms(300);
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

    /* Above the dispatcher, so nothing drains while this runs. */
    presence_adapter_test_outrank_dispatcher(true);

    for (unsigned i = 0; i < PRESENCE_QUEUE_DEPTH + 2u; i++) {
        presence_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.kind = PRESENCE_EV_TICK;
        if (!presence_adapter_post(&ev)) {
            refused++;
        }
    }

    presence_adapter_test_outrank_dispatcher(false);
    presence_adapter_sleep_ms(50);

    /* HOW MANY were refused is the kernel's business, and an earlier version of this
     * file asserted exactly two, which is what FreeRTOS does and is not what Zephyr
     * does. k_msgq_put hands a message straight to a thread already waiting in
     * k_msgq_get, bypassing the buffer, so with the dispatch thread pending the queue
     * absorbs one more than its depth and only one post past the end is refused.
     * FreeRTOS copies into the queue storage first and then unblocks the receiver, so
     * both are refused. Neither is wrong, and the number is printed rather than
     * asserted so the log records which kernel did what.
     *
     * WHAT IS ASSERTED is the thing this phase exists for and the thing that is the
     * same everywhere: a queue that cannot take an event refuses it, and the refusal
     * is counted rather than lost in silence. An adapter that dropped quietly would
     * pass every other phase in this file. */
    printf("   depth %u, posted %u, refused %u\n",
           (unsigned)PRESENCE_QUEUE_DEPTH,
           (unsigned)(PRESENCE_QUEUE_DEPTH + 2u), refused);
    CHECK(refused >= 1u,
          "posting %u past a queue of %u should refuse at least one, refused %u",
          2u, (unsigned)PRESENCE_QUEUE_DEPTH, refused);
    CHECK(presence_adapter_events_dropped() == refused,
          "every refusal must be counted: the adapter says %u and %u were refused",
          (unsigned)presence_adapter_events_dropped(), refused);
}

static void phase_the_heartbeat(void)
{
    uint32_t before;

    printf("4. the heartbeat, started last so the phases above stay quiet\n");
    before = presence_adapter_context()->row_taken[3];   /* free, tick */
    CHECK(presence_adapter_start_tick(20u), "the heartbeat should start");
    presence_adapter_sleep_ms(120);
    CHECK(presence_adapter_context()->row_taken[3] > before,
          "a tick should have arrived in free");
}

/* ----------------------------------------------------------------------- the run */

int presence_phases_run(void)
{
    failures = 0;
    presence_adapter_set_trace(record);

    phase_the_scripted_sequence();
    phase_the_timer_releases();
    phase_a_full_queue_is_counted();
    phase_the_heartbeat();

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    fflush(stdout);
    return failures;
}
