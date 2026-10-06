/* projects/02-the-claim/adapter/claim_adapter.h: what every kernel adapter must provide.
 *
 * THE CONTRACT, AND WHY IT IS A FILE RATHER THAN A CONVENTION. P01 learned this by having
 * its first adapter's header carry a `long *higher_priority_task_woken` on the interrupt
 * post, which is how FreeRTOS asks a caller to yield and is not a thing Zephyr has. One
 * kernel's calling convention had been written into a header that three kernels must
 * satisfy, and the second implementation is what exposed it. So this header is written to
 * be satisfied rather than to describe what one kernel happens to do, and every adapter
 * includes it.
 *
 * WHAT THE ADAPTER OWNS: a queue, a dispatch thread, a grace timer, and the claim_t that
 * ../c/claim.c operates on. What it does NOT own is the policy. Every decision comes from
 * claim_decide, which knows no kernel and no clock.
 *
 * THE SHAPE DIFFERS FROM P01'S ADAPTER BECAUSE THE CHAPTER DOES. P01's machine consumes
 * events and advances a state. This one's cascade is a pure function of a set of inputs, so
 * the adapter holds those inputs and an event changes one of them. The decision is then
 * recomputed from the whole input set rather than advanced from the previous one, which is
 * what "a claim is a decision, not a state" means when it reaches a kernel.
 */
#ifndef CLAIM_ADAPTER_H
#define CLAIM_ADAPTER_H

#include "claim.h"

/* Sixteen, as in P01, so the two projects' queue arithmetic is comparable. */
#define CLAIM_QUEUE_DEPTH 16u

/* ---------------------------------------------------------------- the inbound event */

typedef enum {
    CLAIM_EV_WINDOW        = 0,   /* a calendar window opened or closed */
    CLAIM_EV_PRESENCE      = 1,   /* P01's machine says the room is in use, or is not */
    CLAIM_EV_PRESS         = 2,   /* a panel long press became pending, or was cleared */
    CLAIM_EV_SERVICE       = 3,   /* the service axis changed */
    CLAIM_EV_GRACE_EXPIRED = 4,   /* the grace timer fired. Only the adapter posts this */
    CLAIM_EV_ACTIVATE      = 5,   /* commissioning, which is a setting and not a reading */
    CLAIM_EV_SET_GRACE     = 6,   /* the grace period, in milliseconds for a test's sake */
    CLAIM_EV_KIND_COUNT    = 7
} claim_event_kind_t;

typedef struct {
    uint32_t at_ms;
    uint8_t  kind;
    union {
        bool     flag;        /* window, presence, press, activate */
        uint32_t grace_ms;    /* set_grace */
        struct {
            uint8_t state;    /* claim_service_t */
            uint8_t reason;   /* claim_svc_reason_t */
        } svc;
    } u;
} claim_input_event_t;

/* ------------------------------------------------------------------- the contract */

/* Create the queue, the dispatch thread and the grace timer, and zero the context. */
bool claim_adapter_init(void);

/* From a thread. False means the queue was full and the event is lost, which is counted. */
bool claim_adapter_post(const claim_input_event_t *ev);

/* From an interrupt. NO out-parameter and no yield flag: that was one kernel's calling
 * convention and each adapter now yields on its own behalf. */
bool claim_adapter_post_from_isr(const claim_input_event_t *ev);

/* Every refusal is counted. An adapter that dropped quietly would pass every phase that
 * does not count, which is why this is in the contract rather than in one adapter. */
uint32_t claim_adapter_events_dropped(void);

const claim_t   *claim_adapter_context(void);
claim_decision_t claim_adapter_last(void);

/* Called after every dispatch, with the decision and the context after it. */
typedef void (*claim_trace_fn)(const claim_decision_t *d, const claim_t *after);
void claim_adapter_set_trace(claim_trace_fn fn);

/* The three indicators show the CLAIM here, not the presence, which is chapter 02's
 * visible difference from chapter 01. On a host build this prints. */
void claim_indicators(claim_code_t code);

/* ------------------------------------------------- the two the phases cannot do without */

/* The phases include no kernel header, so sleeping and outranking the dispatcher are the
 * two things they must ask the adapter for. Everything else they do through the contract
 * above. */
void claim_adapter_sleep_ms(uint32_t ms);

/* Raise this thread above the dispatcher, so that a phase can fill the queue without it
 * draining. Only a test calls this. */
void claim_adapter_test_outrank_dispatcher(bool outrank);

#endif /* CLAIM_ADAPTER_H */
