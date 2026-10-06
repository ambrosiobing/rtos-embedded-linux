/* projects/02-the-claim/c/claim.h: the claim cascade, and nothing that knows a kernel.
 *
 * THE SPECIFICATION IS docs/DESIGN.md, which was committed before this file existed.
 * What is written here is that page in C, and where the two disagree the page is right
 * until somebody changes it on purpose.
 *
 * THE SHAPE, and it differs from P01 deliberately. P01 is a transition table because
 * presence is a state machine. A claim is a decision, so this is a CASCADE: eight arms
 * evaluated top to bottom, the first match winning, as one pure function. Seven arms are
 * guarded and the eighth is not, which is why it is last.
 *
 * THE ARM NUMBER IS PART OF THE OUTPUT, not only the code. P01's rows 0 and 1 shared a
 * state and an event and differed only by a guard, so swapping them left every final
 * state identical and a test of outcomes alone would have passed against the defect.
 * claim_decide therefore returns which arm fired, and the test asserts it.
 *
 * NO KERNEL, NO ALLOCATION, NO CLOCK. The cascade is given its inputs and returns a
 * decision. The only time it knows is the seconds stamp handed to claim_step for the
 * record, and even that it only copies.
 */
#ifndef CLAIM_H
#define CLAIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ the claim axis */

/* Chapter 02's seven codes are 0 to 6. CLAIM_FREE is the fall-through, the absence of a
 * claim, and is deliberately NOT one of the seven: an ordinary release is told from a
 * no-show by the code and not by a separate field that could be dropped. */
typedef enum {
    CLAIM_INVISIBLE = 0,
    CLAIM_REJECTED  = 1,
    CLAIM_BOOKED    = 2,
    CLAIM_GRACE     = 3,
    CLAIM_NO_SHOW   = 4,
    CLAIM_WALKIN    = 5,
    CLAIM_BRB       = 6,
    CLAIM_FREE      = 7,
    CLAIM_CODE_COUNT = 8
} claim_code_t;

#define CLAIM_PUBLISHED_CODES 7u   /* the seven; CLAIM_FREE is not among them */
#define CLAIM_ARM_COUNT       8u   /* arms are numbered 1 to 8, as in DESIGN.md */

/* ---------------------------------------------------------------- the service axis */

typedef enum {
    SVC_OK             = 0,
    SVC_DEGRADED       = 1,
    SVC_NEEDS_SERVICE  = 2,
    SVC_OUT_OF_SERVICE = 3,
    SVC_IN_MAINTENANCE = 4,
    SVC_NEEDS_PART     = 5,
    SVC_REPLACED       = 6,
    SVC_STATE_COUNT    = 7
} claim_service_t;

/* SVC_REASON_NONE is not one of the chapter's seven either, for the same reason. */
typedef enum {
    SVC_REASON_NONE             = 0,
    SVC_REASON_SENSOR           = 1,
    SVC_REASON_MODEM            = 2,
    SVC_REASON_PANEL            = 3,
    SVC_REASON_POWER            = 4,
    SVC_REASON_FAN              = 5,
    SVC_REASON_SETUP_INCOMPLETE = 6,
    SVC_REASON_COMMANDED        = 7,
    SVC_REASON_COUNT            = 8
} claim_svc_reason_t;

#define SVC_REASONS 7u

/* ---------------------------------------------------------------------- the inputs */

typedef struct {
    bool               activated;
    bool               window_open;
    bool               present;
    bool               past_grace;
    bool               long_press_pending;
    claim_service_t    service;
    claim_svc_reason_t service_reason;
} claim_inputs_t;

typedef struct {
    claim_code_t code;
    uint8_t      arm;          /* 1 to 8 */
} claim_decision_t;

/* ------------------------------------------------------------------- the settings */

typedef struct {
    uint32_t grace_s;
    uint32_t brb_hold_s;
    uint32_t walkin_len_s;
    uint32_t spool_bytes;
    uint32_t stale_after_s;
    bool     activated;
} claim_settings_t;

#define CLAIM_GRACE_S_DEFAULT       300u
#define CLAIM_BRB_HOLD_S_DEFAULT    900u
#define CLAIM_WALKIN_LEN_S_DEFAULT 1800u
#define CLAIM_SPOOL_BYTES_DEFAULT  4096u
#define CLAIM_STALE_AFTER_S_DEFAULT 600u

/* ---------------------------------------------------------------------- the spool */

/* Every field is fixed width and ordered largest first so that the size is 8 bytes with
 * no padding on any target this project builds for. The test prints sizeof and the
 * capacity it implies, because chapter 01's memory budget was wrong by a factor of five
 * while three of its rows claimed "by construction". */
typedef struct {
    uint32_t at_s;
    uint8_t  code;             /* claim_code_t, narrowed to fix the size */
    uint8_t  arm;
    uint8_t  service;          /* claim_service_t */
    uint8_t  service_reason;   /* claim_svc_reason_t */
} claim_event_t;

/* The bound is in BYTES, because bytes are what run out. The array is sized for the
 * largest bound this build admits; claim_spool_init narrows the usable capacity to
 * whatever the settings ask for. */
#define CLAIM_SPOOL_BYTES_MAX 4096u
#define CLAIM_SPOOL_SLOTS     (CLAIM_SPOOL_BYTES_MAX / sizeof(claim_event_t))

typedef struct {
    claim_event_t slot[CLAIM_SPOOL_SLOTS];
    size_t        cap;         /* usable slots, from the byte bound */
    size_t        head;        /* oldest */
    size_t        count;
    uint32_t      discarded;
    uint32_t      offered;
} claim_spool_t;

/* -------------------------------------------------------------------- the context */

typedef struct {
    claim_settings_t settings;
    claim_inputs_t   last_in;
    claim_decision_t last;
    claim_code_t     published;   /* the last code that actually left the unit */
    claim_spool_t    spool;
    uint32_t         emitted;
    uint32_t         arm_taken[CLAIM_ARM_COUNT];
    uint32_t         code_seen[CLAIM_CODE_COUNT];
    uint32_t         service_seen[SVC_STATE_COUNT];
    uint32_t         reason_seen[SVC_REASON_COUNT];
} claim_t;

/* ------------------------------------------------------------------------- the API */

void claim_settings_defaults(claim_settings_t *s);

/* Four of the seven service states block a claim and three do not. DEGRADED and
 * NEEDS_SERVICE are maintenance facts: something should be looked at and the room still
 * works. The argument is in DESIGN.md. */
bool claim_service_blocks(claim_service_t svc);

/* The cascade. Pure: it reads *in and returns a decision, and touches nothing. */
claim_decision_t claim_decide(const claim_inputs_t *in);

/* An event leaves the unit when the code changes, and GRACE never leaves at all. The
 * calendar already knows about the booking because it sent it; what the unit knows and
 * the calendar cannot is whether anybody came, and during grace it has observed nothing
 * on that question. */
bool claim_emits(claim_code_t prev, claim_code_t now);

void claim_init(claim_t *c);

/* Decide, record the coverage, and spool an event if one is due. Returns the decision. */
claim_decision_t claim_step(claim_t *c, const claim_inputs_t *in, uint32_t at_s);

/* The six invariants of DESIGN.md, checked together. The host test calls this after
 * every single case rather than at the end, because a broken invariant can be transient
 * and still wrong. */
bool claim_check_invariants(const claim_t *c);

void     claim_spool_init(claim_spool_t *sp, uint32_t bytes);
/* Always accepts, discarding the oldest when full, because the newest event is the one
 * describing the room now. Returns true when a discard happened. */
bool     claim_spool_push(claim_spool_t *sp, const claim_event_t *ev);
size_t   claim_spool_capacity(const claim_spool_t *sp);
size_t   claim_spool_count(const claim_spool_t *sp);
uint32_t claim_spool_discarded(const claim_spool_t *sp);

const char *claim_code_name(claim_code_t c);
const char *claim_service_name(claim_service_t s);
const char *claim_svc_reason_name(claim_svc_reason_t r);

size_t claim_event_bytes(void);

#endif /* CLAIM_H */
