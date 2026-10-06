/* projects/01-presence/c/presence.c: the twenty-eight rows, and a dispatcher.
 *
 * The dispatcher is a dozen lines, which is the finding chapter 01 reports about
 * its own prior art: the small permissive state machine libraries each replace
 * about this much code, and a dependency that replaces a dozen lines has to be
 * argued for rather than assumed. The hierarchical framework that would genuinely
 * carry this chapter is under a licence that excludes it from a permissively
 * published portfolio, so it was read and not pasted.
 *
 * ROW ORDER IS LOAD BEARING. Guards are evaluated top to bottom and the first
 * match wins, so rows sharing a from-state and an event are told apart by their
 * order. Row 0 before row 1 is the pair that matters: arrival is checked before
 * the run is counted. Swapping them delays every arrival by one reading and
 * leaves every final state identical, which no test of outcomes alone would
 * catch. The tests therefore assert the row index.
 *
 * C11, and nothing here needs more than C11. The two later variants in
 * variants/ exist to show what C17 and C23 change about expressing the same
 * table, which is less than their release notes suggest, and that is the honest
 * finding rather than a disappointment.
 */
#include "presence.h"

#include <string.h>

/* A guard returns true when its row applies. NULL means an unguarded row, which
 * always applies, and the table is written so that an unguarded row is last
 * among the rows sharing its state and event. */
typedef bool (*presence_guard_fn)(const presence_t *p,
                                  const presence_event_t *ev);
typedef void (*presence_action_fn)(presence_t *p, const presence_event_t *ev);

typedef struct {
    presence_state_t from;
    presence_event_kind_t event;
    presence_guard_fn guard;
    presence_action_fn action;
    presence_state_t to;
    const char *name;        /* for the log line and for test failure messages */
} presence_row_t;

/* ------------------------------------------------------------------ guards */

static bool in_range(const presence_t *p, const presence_event_t *ev)
{
    return ev->u.range_mm <= p->settings.range_mm_max;
}

static bool out_of_range(const presence_t *p, const presence_event_t *ev)
{
    return !in_range(p, ev);
}

/* In range AND this reading completes the run. Checked before the plain
 * in-range row, so the arrival happens on the completing event. */
static bool run_completes(const presence_t *p, const presence_event_t *ev)
{
    if (!in_range(p, ev)) {
        return false;
    }
    /* The run is incremented by the action, so the guard asks whether the next
     * value would reach the threshold. arrive_runs of 0 or 1 arrives at once. */
    uint32_t next = (uint32_t)p->run + 1u;
    return next >= (uint32_t)(p->settings.arrive_runs ? p->settings.arrive_runs
                                                      : 1u);
}

/* ----------------------------------------------------------------- actions */

static void act_none(presence_t *p, const presence_event_t *ev)
{
    (void)p;
    (void)ev;
}

static void act_count_run(presence_t *p, const presence_event_t *ev)
{
    if (p->run < UINT16_MAX) {
        p->run++;
    }
    p->last_reading_ms = ev->at_ms;
}

static void act_reset_run(presence_t *p, const presence_event_t *ev)
{
    p->run = 0;
    p->last_reading_ms = ev->at_ms;
}

static void act_on_arrive(presence_t *p, const presence_event_t *ev)
{
    p->run = 0;                  /* the run has done its job */
    p->hold_running = false;
    p->last_reading_ms = ev->at_ms;
}

static void act_refresh(presence_t *p, const presence_event_t *ev)
{
    p->last_reading_ms = ev->at_ms;
}

static void act_start_hold(presence_t *p, const presence_event_t *ev)
{
    p->hold_running = true;
    p->hold_due_ms = ev->at_ms + p->settings.hold_ms;
    p->last_reading_ms = ev->at_ms;
}

static void act_cancel_hold(presence_t *p, const presence_event_t *ev)
{
    p->hold_running = false;
    p->last_reading_ms = ev->at_ms;
}

/* Row 17. The only release in the table. */
static void act_release(presence_t *p, const presence_event_t *ev)
{
    (void)ev;
    p->hold_running = false;
    p->run = 0;
    p->releases++;
}

static void act_force_occupied(presence_t *p, const presence_event_t *ev)
{
    (void)ev;
    p->run = 0;
    p->hold_running = false;
}

static void act_force_free(presence_t *p, const presence_event_t *ev)
{
    (void)ev;
    p->run = 0;
    p->hold_running = false;
}

static void act_latch_fault(presence_t *p, const presence_event_t *ev)
{
    (void)ev;
    p->hold_running = false;
    p->run = 0;
    p->faults_latched++;
}

static void act_clear_fault(presence_t *p, const presence_event_t *ev)
{
    (void)ev;
    p->run = 0;
    p->hold_running = false;
}

static void act_apply_settings(presence_t *p, const presence_event_t *ev)
{
    p->settings = ev->u.settings;
    if (p->settings.arrive_runs == 0u) {
        p->settings.arrive_runs = 1u;
    }
}

/* ------------------------------------------------------------------- table */

#define ROW(from, ev, guard, action, to) \
    { (from), (ev), (guard), (action), (to), #from " " #ev " " #guard }

static const presence_row_t TABLE[] = {
    /* 0 to 7: FREE */
    ROW(PRESENCE_FREE, PRESENCE_EV_READING, run_completes, act_on_arrive,
        PRESENCE_OCCUPIED),
    ROW(PRESENCE_FREE, PRESENCE_EV_READING, in_range, act_count_run,
        PRESENCE_FREE),
    ROW(PRESENCE_FREE, PRESENCE_EV_READING, out_of_range, act_reset_run,
        PRESENCE_FREE),
    ROW(PRESENCE_FREE, PRESENCE_EV_TICK, NULL, act_none, PRESENCE_FREE),
    /* A stale hold timer. The kernel timer can already be queued when a hold is
     * cancelled or forced away, so a TIMEOUT genuinely arrives in a state that
     * is no longer holding. Without this row the dispatcher would return
     * ERR_NO_ROW for a legitimate race, and a dropped event is how a release
     * goes missing. It does nothing on purpose: the hold it belonged to is
     * already resolved. */
    ROW(PRESENCE_FREE, PRESENCE_EV_TIMEOUT, NULL, act_none, PRESENCE_FREE),
    ROW(PRESENCE_FREE, PRESENCE_EV_BUTTON, NULL, act_force_occupied,
        PRESENCE_OCCUPIED),
    ROW(PRESENCE_FREE, PRESENCE_EV_FAULT, NULL, act_latch_fault,
        PRESENCE_FAULT),
    ROW(PRESENCE_FREE, PRESENCE_EV_SETTINGS, NULL, act_apply_settings,
        PRESENCE_FREE),

    /* 8 to 14: OCCUPIED */
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_READING, in_range, act_refresh,
        PRESENCE_OCCUPIED),
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_READING, out_of_range, act_start_hold,
        PRESENCE_HELD),
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_TICK, NULL, act_none,
        PRESENCE_OCCUPIED),
    /* The same stale timer, and this is the one that actually happens: row 15
     * cancels the hold when a reading returns, by which time the kernel may
     * already have queued the expiry. */
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_TIMEOUT, NULL, act_none,
        PRESENCE_OCCUPIED),
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_BUTTON, NULL, act_force_free,
        PRESENCE_FREE),
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_FAULT, NULL, act_latch_fault,
        PRESENCE_FAULT),
    ROW(PRESENCE_OCCUPIED, PRESENCE_EV_SETTINGS, NULL, act_apply_settings,
        PRESENCE_OCCUPIED),

    /* 15 to 21: HELD */
    ROW(PRESENCE_HELD, PRESENCE_EV_READING, in_range, act_cancel_hold,
        PRESENCE_OCCUPIED),
    ROW(PRESENCE_HELD, PRESENCE_EV_READING, out_of_range, act_none,
        PRESENCE_HELD),
    ROW(PRESENCE_HELD, PRESENCE_EV_TIMEOUT, NULL, act_release, PRESENCE_FREE),
    ROW(PRESENCE_HELD, PRESENCE_EV_TICK, NULL, act_none, PRESENCE_HELD),
    ROW(PRESENCE_HELD, PRESENCE_EV_BUTTON, NULL, act_force_free,
        PRESENCE_FREE),
    ROW(PRESENCE_HELD, PRESENCE_EV_FAULT, NULL, act_latch_fault,
        PRESENCE_FAULT),
    ROW(PRESENCE_HELD, PRESENCE_EV_SETTINGS, NULL, act_apply_settings,
        PRESENCE_HELD),

    /* 22 to 27: FAULT. Every event is a row, so the table is total and a fault
     * is never left by anything but the button. A fault that cleared itself on
     * the next good reading would hide the fault that caused it. */
    ROW(PRESENCE_FAULT, PRESENCE_EV_BUTTON, NULL, act_clear_fault,
        PRESENCE_FREE),
    ROW(PRESENCE_FAULT, PRESENCE_EV_TICK, NULL, act_none, PRESENCE_FAULT),
    ROW(PRESENCE_FAULT, PRESENCE_EV_READING, NULL, act_none, PRESENCE_FAULT),
    ROW(PRESENCE_FAULT, PRESENCE_EV_TIMEOUT, NULL, act_none, PRESENCE_FAULT),
    ROW(PRESENCE_FAULT, PRESENCE_EV_FAULT, NULL, act_none, PRESENCE_FAULT),
    ROW(PRESENCE_FAULT, PRESENCE_EV_SETTINGS, NULL, act_apply_settings,
        PRESENCE_FAULT),
};

/* The macro cannot drift from the table: this fails to compile if it does. */
_Static_assert(sizeof(TABLE) / sizeof(TABLE[0]) == PRESENCE_ROW_COUNT,
               "PRESENCE_ROW_COUNT disagrees with the table");

/* The three sizes chapter 01's memory budget quotes, pinned here so that they are
 * compile-time facts rather than arithmetic in a table nobody re-does. A field
 * added to any of these structures fails this build, which is the intent: the
 * budget is then revisited deliberately instead of going quietly stale, which is
 * exactly what happened to it once already.
 *
 * These hold on the host and on the part. arm-none-eabi-gcc defaults to
 * -fshort-enums because AAPCS requires it, so `kind` and `state` are one byte on
 * the board and four on the host; in both structures the padding before the next
 * uint32_t absorbs the difference and the totals do not move. That is worth
 * asserting rather than assuming, because it is luck rather than design. */
_Static_assert(sizeof(presence_settings_t) == 12u,
               "the settings are no longer 12 bytes; revisit the memory budget");
_Static_assert(sizeof(presence_event_t) == 20u,
               "an event is no longer 20 bytes; every kernel queue is sized from "
               "this and the budget in the chapter quotes it");
_Static_assert(sizeof(presence_t) == 152u,
               "the context is no longer 152 bytes; revisit the memory budget");

size_t presence_row_count(void)
{
    return sizeof(TABLE) / sizeof(TABLE[0]);
}

size_t presence_table_bytes(void)
{
    return sizeof(TABLE);
}

/* -------------------------------------------------------------- dispatcher */

void presence_init(presence_t *p, const presence_settings_t *s)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->state = PRESENCE_FREE;
    p->last_row = -1;
    if (s != NULL) {
        p->settings = *s;
    } else {
        p->settings.arrive_runs = PRESENCE_DEFAULT_ARRIVE_RUNS;
        p->settings.hold_ms = PRESENCE_DEFAULT_HOLD_MS;
        p->settings.range_mm_max = PRESENCE_DEFAULT_RANGE_MM_MAX;
    }
    if (p->settings.arrive_runs == 0u) {
        p->settings.arrive_runs = 1u;
    }
}

presence_result_t presence_dispatch(presence_t *p, const presence_event_t *ev)
{
    if (p == NULL || ev == NULL) {
        return PRESENCE_ERR_ARGS;
    }
    if ((unsigned)p->state >= (unsigned)PRESENCE_STATE_COUNT) {
        return PRESENCE_ERR_BAD_STATE;
    }
    if ((unsigned)ev->kind >= (unsigned)PRESENCE_EV_COUNT) {
        return PRESENCE_ERR_BAD_EVENT;
    }

    for (size_t i = 0; i < presence_row_count(); i++) {
        const presence_row_t *row = &TABLE[i];
        if (row->from != p->state || row->event != ev->kind) {
            continue;
        }
        if (row->guard != NULL && !row->guard(p, ev)) {
            continue;
        }
        row->action(p, ev);
        p->state = row->to;
        p->last_row = (int16_t)i;
        p->row_taken[i]++;
        return PRESENCE_OK;
    }

    /* The table is total, so this is unreachable unless a row was deleted. It is
     * an error rather than a silent drop, because a dropped event is how a
     * release goes missing. */
    p->last_row = -1;
    return PRESENCE_ERR_NO_ROW;
}

bool presence_is_occupied(const presence_t *p)
{
    if (p == NULL) {
        return false;
    }
    return p->state == PRESENCE_OCCUPIED || p->state == PRESENCE_HELD;
}

bool presence_check_invariants(const presence_t *p)
{
    if (p == NULL) {
        return false;
    }
    /* A hold is outstanding only while held. An orphaned hold in any other state
     * is a timer that will fire into a state with no row for it. */
    if (p->hold_running && p->state != PRESENCE_HELD) {
        return false;
    }
    /* Held without a hold is the lost release: nothing will ever return it to
     * free, and it will report occupied for ever. */
    if (p->state == PRESENCE_HELD && !p->hold_running) {
        return false;
    }
    /* A run counts towards an arrival and means nothing once arrived. */
    if (p->run != 0u && p->state != PRESENCE_FREE) {
        return false;
    }
    /* Fault is a latch, so it cannot hold a run or a timer. */
    if (p->state == PRESENCE_FAULT && (p->run != 0u || p->hold_running)) {
        return false;
    }
    if (p->settings.arrive_runs == 0u) {
        return false;
    }
    return true;
}

const char *presence_state_name(presence_state_t s)
{
    switch (s) {
    case PRESENCE_FREE:     return "free";
    case PRESENCE_OCCUPIED: return "occupied";
    case PRESENCE_HELD:     return "held";
    case PRESENCE_FAULT:    return "fault";
    case PRESENCE_STATE_COUNT:
    default:                return "invalid";
    }
}

const char *presence_event_name(presence_event_kind_t k)
{
    switch (k) {
    case PRESENCE_EV_TICK:     return "tick";
    case PRESENCE_EV_READING:  return "reading";
    case PRESENCE_EV_TIMEOUT:  return "timeout";
    case PRESENCE_EV_BUTTON:   return "button";
    case PRESENCE_EV_FAULT:    return "fault";
    case PRESENCE_EV_SETTINGS: return "settings";
    case PRESENCE_EV_COUNT:
    default:                   return "invalid";
    }
}
