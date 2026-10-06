/* projects/02-the-claim/adapter/phases.c: four phases, and NO kernel header.
 *
 * WHAT THIS PROVES THAT ../c/test_claim.c DOES NOT. That file calls claim_decide and
 * claim_step directly, which is the right way to test a pure function and says nothing
 * about a kernel. This one posts the same changes into a real queue, lets a real dispatch
 * thread take them, and lets a real timer expire the grace period, then asserts the same
 * decisions come out. Same policy, different plumbing.
 *
 * AND IT INCLUDES NO KERNEL HEADER. Everything it needs beyond the adapter contract is two
 * calls, claim_adapter_sleep_ms and claim_adapter_test_outrank_dispatcher, which each
 * adapter supplies. That is what lets one test run against every kernel rather than each
 * kernel having a test of its own that nobody can compare.
 *
 * THE FOUR, and the second and fourth are the ones worth having.
 *
 *   1. The scripted sequence through the queue, which is a walk through the chapter's own
 *      story: a window opens on an empty room, the holder arrives, they leave, the room is
 *      free, somebody else walks in. Then the power fails and the room reports itself
 *      rejected rather than occupied, which is the service axis outranking the booking
 *      arms, and the only place in this project where both axes are driven through a
 *      kernel.
 *   2. A no-show produced by a REAL TIMER rather than by an injected past_grace. Every
 *      no-show elsewhere in this project is one a test wrote; this is the only one a clock
 *      caused.
 *   3. A full queue, which is the one failure this design cannot tolerate and cannot detect
 *      afterwards. HOW MANY are refused is the kernel's business and is printed rather than
 *      asserted: P01 asserted two, which is true of FreeRTOS and false of Zephyr.
 *   4. The spool keeps the record while the link is down, which is chapter 02's own
 *      criterion and has no counterpart in chapter 01.
 */
#include "phases.h"

#include "claim_adapter.h"

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
static uint8_t seen_code[TRACE_MAX];
static uint8_t seen_arm[TRACE_MAX];
static unsigned seen_count;

static void record(const claim_decision_t *d, const claim_t *after)
{
    CHECK(claim_check_invariants(after),
          "the invariants broke after arm %u", (unsigned)d->arm);
    if (seen_count < TRACE_MAX) {
        seen_code[seen_count] = (uint8_t)d->code;
        seen_arm[seen_count] = d->arm;
    }
    seen_count++;
}

/* ------------------------------------------------------------------ the posting */

static void post(claim_event_kind_t kind, bool flag)
{
    claim_input_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.kind = (uint8_t)kind;
    ev.u.flag = flag;
    CHECK(claim_adapter_post(&ev), "a %u event was refused by the queue", (unsigned)kind);
}

static void post_grace_ms(uint32_t ms)
{
    claim_input_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.kind = (uint8_t)CLAIM_EV_SET_GRACE;
    ev.u.grace_ms = ms;
    CHECK(claim_adapter_post(&ev), "a grace setting was refused by the queue");
}

static void post_service(claim_service_t state, claim_svc_reason_t reason)
{
    claim_input_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.kind = (uint8_t)CLAIM_EV_SERVICE;
    ev.u.svc.state = (uint8_t)state;
    ev.u.svc.reason = (uint8_t)reason;
    CHECK(claim_adapter_post(&ev), "a service change was refused by the queue");
}

static const char *code_of(unsigned i)
{
    return claim_code_name((claim_code_t)seen_code[i]);
}

/* -------------------------------------------------------------------- the phases */

static void phase_the_scripted_sequence(void)
{
    /* The chapter's own story, in order: commissioned, a window opens on an empty room,
     * the holder arrives, the window closes with them still there, they leave, somebody
     * else walks in. */
    static const uint8_t want_code[] = {
        CLAIM_FREE,     /* activated, nothing happening */
        CLAIM_GRACE,    /* a window opens and nobody is here yet */
        CLAIM_BOOKED,   /* the holder arrives */
        CLAIM_WALKIN,   /* the window closes and they are still in the room */
        CLAIM_FREE,     /* they leave */
        CLAIM_WALKIN,   /* somebody else walks in */
        CLAIM_REJECTED, /* the power fails, and the service axis outranks the room */
        CLAIM_WALKIN,   /* the power returns, and the room is in use again */
    };
    static const uint8_t want_arm[] = { 8, 4, 3, 6, 8, 6, 2, 6 };
    unsigned i;

    printf("1. the chapter's sequence and both axes, through a real queue and thread\n");
    seen_count = 0;

    post(CLAIM_EV_ACTIVATE, true);
    post(CLAIM_EV_WINDOW, true);
    post(CLAIM_EV_PRESENCE, true);
    post(CLAIM_EV_WINDOW, false);
    post(CLAIM_EV_PRESENCE, false);
    post(CLAIM_EV_PRESENCE, true);

    /* The service axis, which is half of this chapter and which no other phase
     * exercises through a kernel. It outranks the booking arms, so a room in use
     * reports itself rejected rather than occupied, and the presence input is not
     * touched by either post: the two axes run beside each other and neither is a
     * state of the other. */
    post_service(SVC_OUT_OF_SERVICE, SVC_REASON_POWER);
    post_service(SVC_OK, SVC_REASON_NONE);
    claim_adapter_sleep_ms(50);

    CHECK(seen_count == 8u, "eight decisions, saw %u", seen_count);
    for (i = 0; i < 8u && i < seen_count; i++) {
        CHECK(seen_code[i] == want_code[i],
              "step %u was %s, expected %s", i, code_of(i),
              claim_code_name((claim_code_t)want_code[i]));
        CHECK(seen_arm[i] == want_arm[i],
              "step %u took arm %u, expected %u", i,
              (unsigned)seen_arm[i], (unsigned)want_arm[i]);
    }

    /* Grace emitted nothing, so the record is shorter than the decision sequence. That is
     * the chapter's fourth criterion seen from the kernel's side. */
    CHECK(claim_adapter_context()->spool.count < seen_count,
          "the record must be shorter than the decisions, because grace is silent: "
          "%u against %u", (unsigned)claim_adapter_context()->spool.count, seen_count);
}

static void phase_the_timer_causes_the_no_show(void)
{
    const claim_t *c;

    printf("2. a no-show caused by the grace timer, not by an injected input\n");
    seen_count = 0;

    /* Shorten the grace period through the adapter's own event, which is where that
     * decision lives, and then leave the room empty with a window open. Nothing else is
     * posted: if NO_SHOW appears it is because a kernel timer fired. */
    post(CLAIM_EV_PRESENCE, false);
    post(CLAIM_EV_WINDOW, false);
    post_grace_ms(60u);
    claim_adapter_sleep_ms(20);

    post(CLAIM_EV_WINDOW, true);
    claim_adapter_sleep_ms(20);
    CHECK(claim_adapter_last().code == CLAIM_GRACE,
          "a window on an empty room is grace, was %s",
          claim_code_name(claim_adapter_last().code));

    claim_adapter_sleep_ms(300);

    c = claim_adapter_context();
    CHECK(claim_adapter_last().code == CLAIM_NO_SHOW,
          "the grace timer should have produced a no-show, the claim is %s",
          claim_code_name(claim_adapter_last().code));
    CHECK(claim_adapter_last().arm == 5u,
          "the no-show arm is 5, arm %u fired", (unsigned)claim_adapter_last().arm);
    CHECK(c->published == CLAIM_NO_SHOW,
          "and the event that left must carry it, carried %s",
          claim_code_name(c->published));
}

static void phase_a_full_queue_is_counted(void)
{
    unsigned refused = 0;
    unsigned i;

    printf("3. a full queue loses an event, and says so\n");

    /* Above the dispatcher, so nothing drains while this runs. */
    claim_adapter_test_outrank_dispatcher(true);

    for (i = 0; i < CLAIM_QUEUE_DEPTH + 2u; i++) {
        claim_input_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.kind = (uint8_t)CLAIM_EV_PRESS;
        ev.u.flag = false;
        if (!claim_adapter_post(&ev)) {
            refused++;
        }
    }

    claim_adapter_test_outrank_dispatcher(false);
    claim_adapter_sleep_ms(50);

    /* HOW MANY were refused is the kernel's business. P01 asserted exactly two here, which
     * is what FreeRTOS does and is not what Zephyr does: k_msgq_put hands a message
     * straight to a thread already waiting in k_msgq_get, so with the dispatcher pending
     * the queue absorbs one more than its depth. Neither is wrong. The number is printed
     * and the relationship is asserted. */
    printf("   depth %u, posted %u, refused %u\n",
           (unsigned)CLAIM_QUEUE_DEPTH, (unsigned)(CLAIM_QUEUE_DEPTH + 2u), refused);
    CHECK(refused >= 1u,
          "posting two past a queue of %u should refuse at least one, refused %u",
          (unsigned)CLAIM_QUEUE_DEPTH, refused);
    CHECK(claim_adapter_events_dropped() == refused,
          "every refusal must be counted: the adapter says %u and %u were refused",
          (unsigned)claim_adapter_events_dropped(), refused);
}

static void phase_the_record_survives_the_radio(void)
{
    const claim_t *c;
    uint32_t before;
    unsigned i;

    printf("4. the claim still changes and the spool still holds it, with no link\n");

    /* There is no link in this project at all, which is the point: nothing in the cascade
     * or the adapter can consult one, so the claim cannot depend on it. What CAN be shown
     * is that the record keeps accumulating and stays bounded while nothing drains it,
     * which is what an outage looks like from inside the unit. */
    c = claim_adapter_context();
    before = c->emitted;

    for (i = 0; i < 200u; i++) {
        post(CLAIM_EV_PRESENCE, (i % 2u) == 0u);
    }
    claim_adapter_sleep_ms(100);

    c = claim_adapter_context();
    CHECK(c->emitted > before,
          "the claim must keep changing with no link: %u events before, %u after",
          (unsigned)before, (unsigned)c->emitted);
    CHECK(c->spool.count <= claim_spool_capacity(&c->spool),
          "the spool must stay inside its bound: %u of %u",
          (unsigned)c->spool.count, (unsigned)claim_spool_capacity(&c->spool));
    CHECK((uint32_t)c->spool.count + c->spool.discarded == c->spool.offered,
          "retained plus discarded must equal offered: %u + %u against %u",
          (unsigned)c->spool.count, (unsigned)c->spool.discarded,
          (unsigned)c->spool.offered);

    printf("   offered %u, retained %u of %u, discarded %u\n",
           (unsigned)c->spool.offered, (unsigned)c->spool.count,
           (unsigned)claim_spool_capacity(&c->spool), (unsigned)c->spool.discarded);
}

/* ----------------------------------------------------------------------- the run */

int claim_phases_run(void)
{
    failures = 0;
    claim_adapter_set_trace(record);

    phase_the_scripted_sequence();
    phase_the_timer_causes_the_no_show();
    phase_a_full_queue_is_counted();
    phase_the_record_survives_the_radio();

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    fflush(stdout);
    return failures;
}
