/* projects/01-presence/c/test_presence.c: every row reachable, and the traps.
 *
 * Chapter 01's deliverable is "a transition table whose every row is proven
 * reachable by a test that runs on the host with no board attached, and a release
 * that cannot be lost". This file is that proof. It needs no kernel, no board and
 * no allocator: it drives events straight into the dispatcher, which is the whole
 * reason presence.c contains no hardware.
 *
 * THREE KINDS OF CASE, and the second and third are the ones that catch things.
 *
 * Coverage: after the sequences below, every one of the twenty-eight rows has
 * been taken at least once. A row never taken is either unreachable, which makes
 * it a defect in the table, or reachable by a path nobody thought to write, which
 * makes it a gap in this file. Either way the final assertion names the row.
 *
 * Order traps: two rows can share a from-state and an event and differ only by
 * their guard, so this file asserts the ROW INDEX taken rather than only the
 * state reached. Rows 0 and 1 are such a pair. Swapping them delays every arrival
 * by one reading and leaves every final state identical, so a test of outcomes
 * alone would pass against a defect that loses a reading from every arrival.
 *
 * The invariant: checked after every single dispatch, not at the end. A violation
 * that appears and clears again would pass an end-of-test check, and the
 * violation that matters here is a lost release, which is exactly the kind that
 * can be transient and still wrong.
 *
 * Built and run by .github/workflows/code.yml under three language versions,
 * because this file and presence.c are the same source under all three.
 */
#include "presence.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static presence_t p;

#define CHECK(cond, ...)                                                      \
    do {                                                                      \
        if (!(cond)) {                                                        \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__);                     \
            printf(__VA_ARGS__);                                              \
            printf("\n");                                                     \
            failures++;                                                       \
        }                                                                     \
    } while (0)

/* Dispatch, then check the invariant. Every event in this file goes through
 * here, so the invariant is checked after every transition rather than once at
 * the end: a lost release can be transient and still wrong. */
static void post(const presence_event_t *ev, int want_row)
{
    presence_result_t r = presence_dispatch(&p, ev);
    CHECK(r == PRESENCE_OK, "dispatch of %s returned %d, not OK",
          presence_event_name(ev->kind), (int)r);
    if (want_row >= 0) {
        CHECK(p.last_row == (int16_t)want_row,
              "expected row %d, took row %d", want_row, (int)p.last_row);
    }
    CHECK(presence_check_invariants(&p),
          "invariant broken after %s: state=%s run=%u hold=%d",
          presence_event_name(ev->kind), presence_state_name(p.state),
          (unsigned)p.run, (int)p.hold_running);
}

static void reading(uint16_t mm, uint32_t at_ms, int want_row)
{
    presence_event_t ev = {0};
    ev.kind = PRESENCE_EV_READING;
    ev.at_ms = at_ms;
    ev.u.range_mm = mm;
    post(&ev, want_row);
}

static void simple(presence_event_kind_t k, uint32_t at_ms, int want_row)
{
    presence_event_t ev = {0};
    ev.kind = k;
    ev.at_ms = at_ms;
    post(&ev, want_row);
}

static void settings(uint16_t runs, uint32_t hold_ms, uint16_t max_mm,
                     uint32_t at_ms, int want_row)
{
    presence_event_t ev = {0};
    ev.kind = PRESENCE_EV_SETTINGS;
    ev.at_ms = at_ms;
    ev.u.settings.arrive_runs = runs;
    ev.u.settings.hold_ms = hold_ms;
    ev.u.settings.range_mm_max = max_mm;
    post(&ev, want_row);
}

#define NEAR 1000u   /* inside range_mm_max */
#define FAR  9000u   /* outside it */

static void start(void)
{
    presence_init(&p, NULL);
    CHECK(p.state == PRESENCE_FREE, "a fresh context must be free");
    CHECK(p.last_row == -1, "a fresh context has taken no row");
    CHECK(presence_check_invariants(&p), "a fresh context must be consistent");
}

/* ------------------------------------------------------- the ordinary sitting */

static void test_a_sitting_with_one_gap(void)
{
    printf("a sitting, with one ordinary gap in the middle\n");
    start();

    /* Arrival needs a run of two, so the first in-range reading counts and the
     * second arrives. Row 1 then row 0, and row 0 BEFORE row 1 in the table is
     * what makes the arrival happen on the completing reading. */
    reading(NEAR, 100, 1);
    CHECK(p.state == PRESENCE_FREE, "one reading is not an arrival");
    CHECK(p.run == 1u, "the run must be 1, is %u", (unsigned)p.run);
    reading(NEAR, 200, 0);
    CHECK(p.state == PRESENCE_OCCUPIED, "two readings arrive");
    CHECK(p.run == 0u, "the run is spent once arrived");

    reading(NEAR, 300, 8);               /* refresh */
    reading(FAR, 400, 9);                /* one gap: hold, not release */
    CHECK(p.state == PRESENCE_HELD, "a gap holds rather than releases");
    CHECK(presence_is_occupied(&p), "held still reports occupied");
    CHECK(p.releases == 0u, "nothing has been released yet");

    reading(NEAR, 500, 15);              /* back in view: cancel the hold */
    CHECK(p.state == PRESENCE_OCCUPIED, "a reading cancels the hold");

    /* Now they really leave. */
    reading(FAR, 600, 9);
    simple(PRESENCE_EV_TIMEOUT, 600 + 30000, 17);
    CHECK(p.state == PRESENCE_FREE, "the hold expiring releases");
    CHECK(p.releases == 1u, "exactly one release, is %u",
          (unsigned)p.releases);
}

/* ------------------------------------------------- the order trap, rows 0 and 1 */

static void test_arrival_happens_on_the_completing_reading(void)
{
    printf("the arrival is on the reading that completes the run, not the next\n");
    start();
    reading(NEAR, 10, 1);
    reading(NEAR, 20, 0);
    CHECK(p.state == PRESENCE_OCCUPIED,
          "an implementation that counted before checking would still be free");

    /* With arrive_runs of 1 the very first reading arrives, which is the edge
     * case that a guard written as `> threshold` rather than `>=` would miss. */
    start();
    settings(1, 1000, 2500, 5, 7);
    reading(NEAR, 10, 0);
    CHECK(p.state == PRESENCE_OCCUPIED, "arrive_runs of 1 arrives at once");
}

/* --------------------------------------------- the release cannot be lost */

static void test_only_one_row_releases(void)
{
    printf("row 17 is the only release, and a hold always has a way out\n");
    start();
    reading(NEAR, 10, 1);
    reading(NEAR, 20, 0);
    reading(FAR, 30, 9);
    CHECK(p.state == PRESENCE_HELD && p.hold_running,
          "a hold must be outstanding while held");

    /* Every event that can arrive while held, and none of them may release
     * except the timeout. */
    simple(PRESENCE_EV_TICK, 40, 18);
    CHECK(p.state == PRESENCE_HELD && p.releases == 0u, "a tick releases nothing");
    reading(FAR, 50, 16);
    CHECK(p.state == PRESENCE_HELD && p.releases == 0u,
          "an out-of-range reading while held releases nothing");
    settings(2, 30000, 2500, 60, 21);
    CHECK(p.state == PRESENCE_HELD && p.releases == 0u,
          "a settings change releases nothing");

    simple(PRESENCE_EV_TIMEOUT, 70, 17);
    CHECK(p.releases == 1u, "the timeout is the release");
}

/* ------------------------------------------------- the stale timer, rows 4, 11 */

static void test_a_timer_can_outlive_its_hold(void)
{
    printf("a stale hold timer arriving in free and in occupied\n");
    start();

    /* Occupied: row 15 cancelled the hold, and the kernel timer was already
     * queued. Without row 11 this would be ERR_NO_ROW for a real race. */
    reading(NEAR, 10, 1);
    reading(NEAR, 20, 0);
    reading(FAR, 30, 9);
    reading(NEAR, 40, 15);
    CHECK(p.state == PRESENCE_OCCUPIED && !p.hold_running, "hold cancelled");
    simple(PRESENCE_EV_TIMEOUT, 50, 11);
    CHECK(p.state == PRESENCE_OCCUPIED,
          "a stale timeout must not disturb an occupied room");
    CHECK(p.releases == 0u, "a stale timeout is not a release");

    /* Free: the button forced the state away from a hold. */
    reading(FAR, 60, 9);
    simple(PRESENCE_EV_BUTTON, 70, 19);
    CHECK(p.state == PRESENCE_FREE && !p.hold_running, "forced free");
    simple(PRESENCE_EV_TIMEOUT, 80, 4);
    CHECK(p.state == PRESENCE_FREE, "a stale timeout in free changes nothing");
}

/* -------------------------------------------------------- the fault latch */

static void test_fault_is_a_latch(void)
{
    printf("a fault leaves only by the button\n");
    start();
    simple(PRESENCE_EV_FAULT, 10, 6);
    CHECK(p.state == PRESENCE_FAULT, "a fault latches from free");
    CHECK(p.faults_latched == 1u, "the latch is counted");

    /* Every other event, and none of them may clear it. A fault that cleared
     * itself on the next good reading would hide the fault that caused it. */
    simple(PRESENCE_EV_TICK, 20, 23);
    reading(NEAR, 30, 24);
    CHECK(p.state == PRESENCE_FAULT,
          "a good reading must NOT clear a fault");
    simple(PRESENCE_EV_TIMEOUT, 40, 25);
    simple(PRESENCE_EV_FAULT, 50, 26);
    settings(2, 30000, 2500, 60, 27);
    CHECK(p.state == PRESENCE_FAULT, "still faulted after every other event");

    simple(PRESENCE_EV_BUTTON, 70, 22);
    CHECK(p.state == PRESENCE_FREE, "the button clears it");
}

/* ------------------------------------------- the rows no other case reaches */

static void test_the_remaining_rows(void)
{
    printf("the rows the sequences above do not reach\n");
    start();

    reading(FAR, 10, 2);                      /* reset a run from free */
    simple(PRESENCE_EV_TICK, 20, 3);
    simple(PRESENCE_EV_BUTTON, 30, 5);        /* forced occupied from free */
    CHECK(p.state == PRESENCE_OCCUPIED, "the button forces occupancy");
    simple(PRESENCE_EV_TICK, 40, 10);
    settings(2, 30000, 2500, 50, 14);
    simple(PRESENCE_EV_FAULT, 60, 13);        /* fault from occupied */
    simple(PRESENCE_EV_BUTTON, 70, 22);

    reading(NEAR, 80, 1);
    reading(NEAR, 90, 0);
    simple(PRESENCE_EV_BUTTON, 100, 12);      /* forced free from occupied */
    CHECK(p.state == PRESENCE_FREE, "the button frees an occupied room");

    reading(NEAR, 110, 1);
    reading(NEAR, 120, 0);
    reading(FAR, 130, 9);
    simple(PRESENCE_EV_FAULT, 140, 20);       /* fault from held */
    CHECK(p.state == PRESENCE_FAULT, "a fault latches from held");
}

/* ------------------------------------------------------ refusals and coverage */

static void test_the_dispatcher_refuses_nonsense(void)
{
    printf("what the dispatcher refuses\n");
    presence_event_t ev = {0};
    ev.kind = PRESENCE_EV_TICK;

    CHECK(presence_dispatch(NULL, &ev) == PRESENCE_ERR_ARGS,
          "a null context is an error");
    start();
    CHECK(presence_dispatch(&p, NULL) == PRESENCE_ERR_ARGS,
          "a null event is an error");

    ev.kind = (presence_event_kind_t)PRESENCE_EV_COUNT;
    CHECK(presence_dispatch(&p, &ev) == PRESENCE_ERR_BAD_EVENT,
          "an event kind past the end is an error, not a dropped event");

    start();
    p.state = (presence_state_t)PRESENCE_STATE_COUNT;
    ev.kind = PRESENCE_EV_TICK;
    CHECK(presence_dispatch(&p, &ev) == PRESENCE_ERR_BAD_STATE,
          "a state past the end is an error");
}

static void test_the_table_length_matches_the_header(void)
{
    printf("the table length and the header agree\n");
    CHECK(presence_row_count() == PRESENCE_ROW_COUNT,
          "the table has %zu rows and the header says %u",
          presence_row_count(), (unsigned)PRESENCE_ROW_COUNT);
}

/* Every row, across every case above. Run last, over an accumulating total. */
static uint32_t coverage[PRESENCE_ROW_COUNT];

static void accumulate(void)
{
    for (size_t i = 0; i < PRESENCE_ROW_COUNT; i++) {
        coverage[i] += p.row_taken[i];
    }
}

int main(void)
{
    struct { const char *name; void (*fn)(void); } cases[] = {
        {"sitting",      test_a_sitting_with_one_gap},
        {"arrival",      test_arrival_happens_on_the_completing_reading},
        {"release",      test_only_one_row_releases},
        {"stale timer",  test_a_timer_can_outlive_its_hold},
        {"fault latch",  test_fault_is_a_latch},
        {"remaining",    test_the_remaining_rows},
        {"refusals",     test_the_dispatcher_refuses_nonsense},
        {"table length", test_the_table_length_matches_the_header},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        cases[i].fn();
        accumulate();
    }

    printf("every row reachable\n");
    unsigned untaken = 0;
    for (size_t i = 0; i < PRESENCE_ROW_COUNT; i++) {
        if (coverage[i] == 0u) {
            printf("  FAIL row %zu was never taken. Either it is unreachable, "
                   "which is a defect in the table, or this file has a gap\n", i);
            untaken++;
            failures++;
        }
    }
    if (untaken == 0u) {
        printf("  all %u rows taken\n", (unsigned)PRESENCE_ROW_COUNT);
    }

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
