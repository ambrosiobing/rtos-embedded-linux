/* projects/02-the-claim/c/claim.c: the cascade, the spool, and no hardware.
 *
 * READ claim.h FIRST, and docs/DESIGN.md before that. The order of the arms below is the
 * specification and not a style: DESIGN.md carries a table of the defect each permutation
 * would cause, and five of the six rows in it are real failures a reader would ship.
 */
#include "claim.h"

/* The size the spool arithmetic depends on. If this ever fails, the capacity printed by
 * the test changes with it and the chapter's numbers are wrong rather than the code. */
_Static_assert(sizeof(claim_event_t) == 8u,
               "the spool event must be 8 bytes, or the byte bound buys a "
               "different number of events than the page says");
_Static_assert(CLAIM_SPOOL_SLOTS == 512u,
               "4096 bytes of 8 gives 512 slots");

/* ------------------------------------------------------------------- the settings */

void claim_settings_defaults(claim_settings_t *s)
{
    s->grace_s       = CLAIM_GRACE_S_DEFAULT;
    s->brb_hold_s    = CLAIM_BRB_HOLD_S_DEFAULT;
    s->walkin_len_s  = CLAIM_WALKIN_LEN_S_DEFAULT;
    s->spool_bytes   = CLAIM_SPOOL_BYTES_DEFAULT;
    s->stale_after_s = CLAIM_STALE_AFTER_S_DEFAULT;

    /* Off, and that is the point. The room is invisible until somebody commissions it,
     * so the failure mode of a half-finished installation is a room nobody can book
     * rather than a room that swallows bookings. */
    s->activated = false;
}

/* --------------------------------------------------------------- the service axis */

bool claim_service_blocks(claim_service_t svc)
{
    switch (svc) {
    case SVC_OUT_OF_SERVICE:
    case SVC_IN_MAINTENANCE:
    case SVC_NEEDS_PART:
    case SVC_REPLACED:
        return true;

    /* These two are maintenance facts and not outages. A room taken out of use because
     * a fan needs cleaning is a room lost for a week to a work order. */
    case SVC_DEGRADED:
    case SVC_NEEDS_SERVICE:
    case SVC_OK:
        return false;

    case SVC_STATE_COUNT:
    default:
        /* An unknown service state is treated as blocking, because the safe answer to
         * "is this room fit to be booked" when the answer is not known is no. */
        return true;
    }
}

/* ------------------------------------------------------- the arms, each one named */

/* Each guard is a named predicate rather than a condition written inline. That costs a
 * line apiece and buys the one thing a three-language comparison needs: the C, the C++
 * and the Rust carry the SAME guard names in the SAME order, so a script can compare the
 * cascades arm for arm instead of trusting that the same policy was written three times.
 * P01's table does it this way and its cross-check compares guards by name; an inline
 * condition is readable and is not comparable, because the three languages spell the
 * same test differently.
 *
 * The names are deliberately about the SITUATION and not about the outcome:
 * booked_and_waiting says what is true of the room, not that the answer is GRACE. An
 * arm whose guard is named after its own code cannot be checked against anything. */

static bool not_activated(const claim_inputs_t *in)
{
    return !in->activated;
}

static bool service_blocks(const claim_inputs_t *in)
{
    return claim_service_blocks(in->service);
}

static bool booked_and_present(const claim_inputs_t *in)
{
    return in->window_open && in->present;
}

static bool booked_and_waiting(const claim_inputs_t *in)
{
    return in->window_open && !in->present && !in->past_grace;
}

static bool booked_and_nobody_came(const claim_inputs_t *in)
{
    return in->window_open && !in->present && in->past_grace;
}

static bool unbooked_and_present(const claim_inputs_t *in)
{
    return !in->window_open && in->present;
}

static bool press_pending(const claim_inputs_t *in)
{
    return in->long_press_pending;
}

/* -------------------------------------------------------------------- the cascade */

#define ARM(n, code_) ((claim_decision_t){ (code_), (uint8_t)(n) })

claim_decision_t claim_decide(const claim_inputs_t *in)
{
    /* 1. Not commissioned. Above everything, including the service axis: an
     *    uncommissioned unit does not answer at all, which is what the flag is for. */
    if (not_activated(in)) {
        return ARM(1, CLAIM_INVISIBLE);
    }

    /* 2. The service axis blocks. Above the booking arms, or a room with no power would
     *    report itself booked. */
    if (service_blocks(in)) {
        return ARM(2, CLAIM_REJECTED);
    }

    /* 3. A live booking with its holder present. ABOVE WALK-IN, which is the rule this
     *    chapter is named for: a live booking beats a walk-in. */
    if (booked_and_present(in)) {
        return ARM(3, CLAIM_BOOKED);
    }

    /* 4. Booked, nobody here yet, still inside the grace period. Above the no-show arm,
     *    or every booking would release immediately, because past-grace is false before
     *    grace is. This arm never emits; see claim_emits. */
    if (booked_and_waiting(in)) {
        return ARM(4, CLAIM_GRACE);
    }

    /* 5. The grace period ran out with nobody present. */
    if (booked_and_nobody_came(in)) {
        return ARM(5, CLAIM_NO_SHOW);
    }

    /* 6. No booking, somebody is here, and they have taken the room. */
    if (unbooked_and_present(in)) {
        return ARM(6, CLAIM_WALKIN);
    }

    /* 7. Be right back. BELOW walk-in on purpose, which makes it reachable only when
     *    nobody is present, and that is exactly what it is for: it holds the room for
     *    somebody who pressed the button on the way out. A press while they are still in
     *    the room needs no claim of its own, because arm 3 or arm 6 already describes
     *    the room correctly. */
    if (press_pending(in)) {
        return ARM(7, CLAIM_BRB);
    }

    /* 8. The fall-through, and the only arm with no guard, which is why it is last. */
    return ARM(8, CLAIM_FREE);
}

bool claim_emits(claim_code_t prev, claim_code_t now)
{
    if (now == CLAIM_GRACE) {
        return false;
    }
    return now != prev;
}

/* ---------------------------------------------------------------------- the spool */

void claim_spool_init(claim_spool_t *sp, uint32_t bytes)
{
    size_t want;

    sp->head      = 0u;
    sp->count     = 0u;
    sp->discarded = 0u;
    sp->offered   = 0u;

    want = (size_t)(bytes / (uint32_t)sizeof(claim_event_t));
    if (want > CLAIM_SPOOL_SLOTS) {
        want = CLAIM_SPOOL_SLOTS;
    }
    if (want == 0u) {
        want = 1u;   /* a spool of no events would discard everything silently */
    }
    sp->cap = want;
}

bool claim_spool_push(claim_spool_t *sp, const claim_event_t *ev)
{
    bool discarded = false;

    sp->offered++;

    if (sp->count == sp->cap) {
        /* Drop the oldest. The newest event describes the room now, and a spool that
         * refused the new one would keep a record of a room that has moved on. */
        sp->head = (sp->head + 1u) % sp->cap;
        sp->count--;
        sp->discarded++;
        discarded = true;
    }

    sp->slot[(sp->head + sp->count) % sp->cap] = *ev;
    sp->count++;
    return discarded;
}

size_t   claim_spool_capacity(const claim_spool_t *sp)  { return sp->cap; }
size_t   claim_spool_count(const claim_spool_t *sp)     { return sp->count; }
uint32_t claim_spool_discarded(const claim_spool_t *sp) { return sp->discarded; }

/* -------------------------------------------------------------------- the context */

void claim_init(claim_t *c)
{
    size_t i;

    claim_settings_defaults(&c->settings);
    claim_spool_init(&c->spool, c->settings.spool_bytes);

    c->last_in.activated          = false;
    c->last_in.window_open        = false;
    c->last_in.present            = false;
    c->last_in.past_grace         = false;
    c->last_in.long_press_pending = false;
    c->last_in.service            = SVC_OK;
    c->last_in.service_reason     = SVC_REASON_NONE;

    /* There is no decision yet. INVISIBLE is the honest initial value because the unit
     * is not commissioned, and arm 0 says no arm has fired. */
    c->last.code = CLAIM_INVISIBLE;
    c->last.arm  = 0u;
    c->published = CLAIM_INVISIBLE;
    c->emitted   = 0u;

    for (i = 0u; i < CLAIM_ARM_COUNT; i++)    { c->arm_taken[i] = 0u; }
    for (i = 0u; i < CLAIM_CODE_COUNT; i++)   { c->code_seen[i] = 0u; }
    for (i = 0u; i < SVC_STATE_COUNT; i++)    { c->service_seen[i] = 0u; }
    for (i = 0u; i < SVC_REASON_COUNT; i++)   { c->reason_seen[i] = 0u; }
}

claim_decision_t claim_step(claim_t *c, const claim_inputs_t *in, uint32_t at_s)
{
    claim_inputs_t   eff;
    claim_decision_t d;

    /* The activation flag lives in settings, and the inputs carry it so that the cascade
     * stays pure. The settings win, because that is where commissioning is recorded. */
    eff = *in;
    eff.activated = c->settings.activated;

    d = claim_decide(&eff);

    c->arm_taken[d.arm - 1u]++;
    c->code_seen[d.code]++;
    c->service_seen[eff.service]++;
    c->reason_seen[eff.service_reason]++;

    if (claim_emits(c->last.code, d.code)) {
        claim_event_t ev;

        ev.at_s           = at_s;
        ev.code           = (uint8_t)d.code;
        ev.arm            = d.arm;
        ev.service        = (uint8_t)eff.service;
        ev.service_reason = (uint8_t)eff.service_reason;

        (void)claim_spool_push(&c->spool, &ev);
        c->emitted++;
        c->published = d.code;
    }

    c->last_in = eff;
    c->last    = d;
    return d;
}

bool claim_check_invariants(const claim_t *c)
{
    size_t i;

    /* 1. An unactivated unit yields INVISIBLE and nothing else. */
    if (!c->settings.activated && c->last.arm != 0u && c->last.code != CLAIM_INVISIBLE) {
        return false;
    }

    /* 2. A blocking service state always yields REJECTED, whatever the booking inputs
     *    say. Only checked once a decision exists. */
    if (c->last.arm != 0u && c->settings.activated &&
        claim_service_blocks(c->last_in.service) && c->last.code != CLAIM_REJECTED) {
        return false;
    }

    /* 3. GRACE never appears in the record, however often it is the decision. */
    for (i = 0u; i < c->spool.count; i++) {
        const claim_event_t *ev = &c->spool.slot[(c->spool.head + i) % c->spool.cap];

        if (ev->code == (uint8_t)CLAIM_GRACE) {
            return false;
        }
    }

    /* 4. The spool never exceeds its bound. */
    if (c->spool.count > c->spool.cap || c->spool.cap > CLAIM_SPOOL_SLOTS) {
        return false;
    }

    /* 5. Retained plus discarded equals offered. A spool losing events it does not admit
     *    to is the failure this design cannot detect any other way. */
    if ((uint32_t)c->spool.count + c->spool.discarded != c->spool.offered) {
        return false;
    }

    /* 6. Every event that left the unit is in the record or was discarded from it. */
    if (c->emitted != c->spool.offered) {
        return false;
    }

    return true;
}

/* ----------------------------------------------------------------------- the names */

const char *claim_code_name(claim_code_t c)
{
    switch (c) {
    case CLAIM_INVISIBLE: return "invisible";
    case CLAIM_REJECTED:  return "rejected";
    case CLAIM_BOOKED:    return "booked";
    case CLAIM_GRACE:     return "grace";
    case CLAIM_NO_SHOW:   return "no-show";
    case CLAIM_WALKIN:    return "walk-in";
    case CLAIM_BRB:       return "be-right-back";
    case CLAIM_FREE:      return "free";
    case CLAIM_CODE_COUNT:
    default:              return "?";
    }
}

const char *claim_service_name(claim_service_t s)
{
    switch (s) {
    case SVC_OK:             return "ok";
    case SVC_DEGRADED:       return "degraded";
    case SVC_NEEDS_SERVICE:  return "needs-service";
    case SVC_OUT_OF_SERVICE: return "out-of-service";
    case SVC_IN_MAINTENANCE: return "in-maintenance";
    case SVC_NEEDS_PART:     return "needs-part";
    case SVC_REPLACED:       return "replaced";
    case SVC_STATE_COUNT:
    default:                 return "?";
    }
}

const char *claim_svc_reason_name(claim_svc_reason_t r)
{
    switch (r) {
    case SVC_REASON_NONE:             return "none";
    case SVC_REASON_SENSOR:           return "sensor";
    case SVC_REASON_MODEM:            return "modem";
    case SVC_REASON_PANEL:            return "panel";
    case SVC_REASON_POWER:            return "power";
    case SVC_REASON_FAN:              return "fan";
    case SVC_REASON_SETUP_INCOMPLETE: return "setup-incomplete";
    case SVC_REASON_COMMANDED:        return "commanded";
    case SVC_REASON_COUNT:
    default:                          return "?";
    }
}

size_t claim_event_bytes(void)
{
    return sizeof(claim_event_t);
}
