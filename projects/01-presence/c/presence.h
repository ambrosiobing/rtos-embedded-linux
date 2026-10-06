/* projects/01-presence/c/presence.h: four states, six events, one table.
 *
 * THE TABLE IS THE SPECIFICATION. docs/DESIGN.md holds the same twenty-eight rows
 * in prose, and the rule from chapter 01 is that the two are one object: an edge
 * that is not a row is a defect and not a special case. The most common way an
 * application of this kind becomes untestable is that one condition gets handled
 * with an early return inside an action instead of as a transition. After five of
 * those the diagram no longer describes the program, and nobody notices until a
 * release goes missing in the field.
 *
 * SO THE DISPATCHER REPORTS THE ROW IT TOOK, not only the state it reached. Two
 * rows can share a from-state and an event and be told apart only by their
 * guard, and a test that compared outcomes alone would accept an implementation
 * that ordered those two the other way. Row 0 before row 1 is exactly such a
 * pair: arrival is checked before the run is counted, so a run reaching its
 * threshold transitions on the event that completes it rather than on the next
 * one. Swapping them delays every arrival by one reading and changes no final
 * state, which is why `last_row` exists and why the tests assert it.
 *
 * NO HARDWARE IN THIS FILE, and none in presence.c either. Everything here is
 * arithmetic over an event and a context, which is what lets the same source
 * compile for a host with no board attached, for three kernels, and under three
 * versions of this language. The kernel adapters in ../rtos/ supply a thread, a
 * queue, a timer and three lamps, and none of them is allowed to make a decision.
 *
 * WHAT THE INVARIANT IS. A release cannot be lost. Row 17 is the only transition
 * that releases a hold, and `presence_check_invariants` is the assertion that no
 * reachable path leaves a context reporting occupied with no hold outstanding and
 * no run in progress. A room that forgets to release is worse than a room with no
 * sensor, because a closed door and a lit indicator look the same either way.
 *
 * C11 is the baseline. ../c/variants/ carries the same table under C17 and C23,
 * and docs/LANGUAGE_IDIOMS.md says what each version actually buys here rather
 * than what its release notes advertise.
 */
#ifndef PRESENCE_H
#define PRESENCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The states, in the order the table groups them. */
typedef enum {
    PRESENCE_FREE = 0,
    PRESENCE_OCCUPIED,
    PRESENCE_HELD,
    PRESENCE_FAULT,
    PRESENCE_STATE_COUNT
} presence_state_t;

/* The six event kinds from chapter 01's data-flow figure. */
typedef enum {
    PRESENCE_EV_TICK = 0,
    PRESENCE_EV_READING,
    PRESENCE_EV_TIMEOUT,
    PRESENCE_EV_BUTTON,
    PRESENCE_EV_FAULT,
    PRESENCE_EV_SETTINGS,
    PRESENCE_EV_COUNT
} presence_event_kind_t;

/* The settings, which are configuration and not constants. The defaults are in
 * docs/DESIGN.md and are described there as arguable rather than measured:
 * nobody on this bench has watched a real room for a week. */
#define PRESENCE_DEFAULT_ARRIVE_RUNS  2u
#define PRESENCE_DEFAULT_HOLD_MS      30000u
#define PRESENCE_DEFAULT_RANGE_MM_MAX 2500u

typedef struct {
    uint16_t arrive_runs;     /* readings in a row before an arrival */
    uint32_t hold_ms;         /* how long a hold outlives the last reading */
    uint16_t range_mm_max;    /* above this, nobody is there */
} presence_settings_t;

/* One event. A reading carries a range in millimetres; a settings event carries
 * the replacement settings. The union is deliberate: an event is one thing, and a
 * struct with every field present would let a caller post a reading and a
 * settings change at once, which is not a row in the table. */
typedef struct {
    presence_event_kind_t kind;
    uint32_t at_ms;                 /* the kernel's monotonic time */
    union {
        uint16_t range_mm;          /* PRESENCE_EV_READING */
        presence_settings_t settings; /* PRESENCE_EV_SETTINGS */
    } u;
} presence_event_t;

/* The number of rows in the table. Asserted against the table's real length in
 * presence.c, so this constant cannot drift from it. */
#define PRESENCE_ROW_COUNT 28u

/* How deep the event queue is. The core never touches a queue: this lives here
 * because every kernel adapter sizes its own queue from it, and because the
 * memory budget is then one multiplication rather than a number repeated in four
 * places. Sixteen is chapter 01's figure and is not a measurement: no board has
 * yet reported a high-water mark. */
#define PRESENCE_QUEUE_DEPTH 16u

typedef struct {
    presence_state_t state;
    presence_settings_t settings;

    uint16_t run;              /* consecutive in-range readings, for arrival */
    bool hold_running;         /* a hold timer is outstanding */
    uint32_t hold_due_ms;      /* when it expires, valid while hold_running */
    uint32_t last_reading_ms;

    /* Observability, which is the difference between a diagnosis and a guess.
     * Per-row counts rather than per-state: a state counter cannot tell a
     * refresh from an arrival. */
    uint32_t row_taken[PRESENCE_ROW_COUNT];
    int16_t last_row;          /* the row the last event took, -1 if none */
    uint32_t releases;         /* row 17, the only release */
    uint32_t faults_latched;
} presence_t;

/* Every return a dispatch can make. A dropped event is not one of them: the
 * table is total, so every (state, event) pair has a row, and an event that
 * matched nothing is a defect in the table rather than an input to tolerate. */
typedef enum {
    PRESENCE_OK = 0,
    PRESENCE_ERR_ARGS = -1,
    PRESENCE_ERR_NO_ROW = -2,      /* the table is not total; a defect */
    PRESENCE_ERR_BAD_STATE = -3,
    PRESENCE_ERR_BAD_EVENT = -4,
} presence_result_t;

void presence_init(presence_t *p, const presence_settings_t *s);

/* Dispatch one event. On success `p->last_row` is the row index taken, which is
 * what the tests assert, and the state is the row's destination. */
presence_result_t presence_dispatch(presence_t *p, const presence_event_t *ev);

/* True while the room should be reported as in use. HELD reports occupied: that
 * is the entire purpose of the state. */
bool presence_is_occupied(const presence_t *p);

/* The invariant from docs/DESIGN.md, checkable at any point. Returns true when
 * the context is consistent. A false here is a lost release or an orphaned hold,
 * and the host tests call it after every single dispatch. */
bool presence_check_invariants(const presence_t *p);

/* Names, for the one log line per transition and for the shell. Static storage,
 * no allocation, safe to call from anywhere. */
const char *presence_state_name(presence_state_t s);
const char *presence_event_name(presence_event_kind_t k);

/* How many rows the table actually has, so a test can compare it against
 * PRESENCE_ROW_COUNT rather than trusting the macro. */
size_t presence_row_count(void);

/* How many bytes the table occupies. The row type is private to presence.c, so
 * this is the only way a caller can report the figure the memory budget wants for
 * flash. Read it with care: a row is mostly pointers, so the host figure is close
 * to twice the board's and is not a substitute for the map file. */
size_t presence_table_bytes(void);

#endif /* PRESENCE_H */
