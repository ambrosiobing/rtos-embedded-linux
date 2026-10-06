/* projects/02-the-claim/c/test_claim.c: the twenty-two cases, and the seven criteria.
 *
 * WHAT THIS FILE IS. Chapter 02 lists seven acceptance criteria and says the cases
 * number twenty-two. Each criterion below is a function, named after it, and the
 * twenty-two cases are a table rather than twenty-two functions, because the thing being
 * tested is a pure function of its inputs and a table is how you read one.
 *
 * WHY THE ARM IS ASSERTED AND NOT ONLY THE CODE. Several arms can produce the same code,
 * and two arms produce CLAIM_FREE between them. A permuted cascade can leave every code
 * identical and still be wrong, which is P01's lesson about its rows 0 and 1.
 *
 * NO KERNEL HERE. This is the host test. The adapter phases that run the same decisions
 * through a real queue come later, and like P01's they will include no kernel header.
 */
#include "claim.h"

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

/* ------------------------------------------------------------- the twenty-two cases */

typedef struct {
    const char        *what;
    bool               activated;
    bool               window_open;
    bool               present;
    bool               past_grace;
    bool               long_press;
    claim_service_t    svc;
    claim_svc_reason_t reason;
    claim_code_t       want_code;
    uint8_t            want_arm;
} case_t;

static const case_t CASES[] = {
    /* Arm 1: not commissioned. Four cases, because the whole point of the flag is that
     * it outranks every other input, including a blocking service state. */
    { "uncommissioned and quiet",
      false, false, false, false, false, SVC_OK, SVC_REASON_SETUP_INCOMPLETE,
      CLAIM_INVISIBLE, 1u },
    { "uncommissioned, booked, holder present",
      false, true,  true,  false, false, SVC_OK, SVC_REASON_NONE,
      CLAIM_INVISIBLE, 1u },
    { "uncommissioned, somebody walks in",
      false, false, true,  false, false, SVC_OK, SVC_REASON_NONE,
      CLAIM_INVISIBLE, 1u },
    { "uncommissioned and out of service: activation outranks the service axis",
      false, false, false, false, false, SVC_OUT_OF_SERVICE, SVC_REASON_POWER,
      CLAIM_INVISIBLE, 1u },

    /* Arm 2: the service axis blocks. All four blocking states, and the last one shows
     * that it outranks the no-show arm as well as the booking arms. */
    { "out of service with the holder present",
      true,  true,  true,  false, false, SVC_OUT_OF_SERVICE, SVC_REASON_POWER,
      CLAIM_REJECTED, 2u },
    { "somebody is working in the room",
      true,  false, true,  false, false, SVC_IN_MAINTENANCE, SVC_REASON_COMMANDED,
      CLAIM_REJECTED, 2u },
    { "a part is missing",
      true,  false, false, false, false, SVC_NEEDS_PART, SVC_REASON_FAN,
      CLAIM_REJECTED, 2u },
    { "superseded, and past grace: service outranks the no-show",
      true,  true,  false, true,  false, SVC_REPLACED, SVC_REASON_COMMANDED,
      CLAIM_REJECTED, 2u },

    /* Arm 3: a live booking with its holder present. The second and third show that the
     * two non-blocking fault states do not take the room out of use. */
    { "booked and the holder is here",
      true,  true,  true,  false, false, SVC_OK, SVC_REASON_NONE,
      CLAIM_BOOKED, 3u },
    { "booked and here, with a fan that needs cleaning",
      true,  true,  true,  false, false, SVC_DEGRADED, SVC_REASON_FAN,
      CLAIM_BOOKED, 3u },
    { "booked and here, with a sensor due for service",
      true,  true,  true,  false, false, SVC_NEEDS_SERVICE, SVC_REASON_SENSOR,
      CLAIM_BOOKED, 3u },

    /* Arm 4: booked, nobody here yet, inside the grace period. */
    { "booked and nobody has arrived yet",
      true,  true,  false, false, false, SVC_OK, SVC_REASON_NONE,
      CLAIM_GRACE, 4u },
    { "booked, nobody yet, modem due for service",
      true,  true,  false, false, false, SVC_NEEDS_SERVICE, SVC_REASON_MODEM,
      CLAIM_GRACE, 4u },

    /* Arm 5: the grace period ran out. */
    { "nobody came",
      true,  true,  false, true,  false, SVC_OK, SVC_REASON_NONE,
      CLAIM_NO_SHOW, 5u },
    { "nobody came, and the panel is faulty",
      true,  true,  false, true,  false, SVC_DEGRADED, SVC_REASON_PANEL,
      CLAIM_NO_SHOW, 5u },

    /* Arm 6: a walk-in. The second shows that a press by somebody still in the room does
     * not hide that the room is in use. */
    { "somebody walks into an unbooked room",
      true,  false, true,  false, false, SVC_OK, SVC_REASON_NONE,
      CLAIM_WALKIN, 6u },
    { "a walk-in who presses the button while still in the room",
      true,  false, true,  false, true,  SVC_OK, SVC_REASON_NONE,
      CLAIM_WALKIN, 6u },

    /* Arm 7: be right back, which is reachable only when nobody is present. */
    { "pressed the button on the way out",
      true,  false, false, false, true,  SVC_OK, SVC_REASON_NONE,
      CLAIM_BRB, 7u },
    { "pressed on the way out, with a fan that needs cleaning",
      true,  false, false, false, true,  SVC_DEGRADED, SVC_REASON_FAN,
      CLAIM_BRB, 7u },

    /* Arm 8: the fall-through, which is not one of the seven codes. */
    { "nothing at all is happening",
      true,  false, false, false, false, SVC_OK, SVC_REASON_NONE,
      CLAIM_FREE, 8u },
    { "nothing happening, sensor due for service: still free",
      true,  false, false, false, false, SVC_NEEDS_SERVICE, SVC_REASON_SENSOR,
      CLAIM_FREE, 8u },

    /* And the one that shows grace outranks a press: somebody pressed the panel while
     * the room was booked and empty, which creates no claim of its own. */
    { "a press during grace creates no claim",
      true,  true,  false, false, true,  SVC_OK, SVC_REASON_NONE,
      CLAIM_GRACE, 4u },
};

#define CASE_COUNT (sizeof(CASES) / sizeof(CASES[0]))

static claim_inputs_t inputs_of(const case_t *k)
{
    claim_inputs_t in;

    memset(&in, 0, sizeof(in));
    in.activated          = k->activated;
    in.window_open        = k->window_open;
    in.present            = k->present;
    in.past_grace         = k->past_grace;
    in.long_press_pending = k->long_press;
    in.service            = k->svc;
    in.service_reason     = k->reason;
    return in;
}

/* ---------------------------------------- 1. every code, every arm, every reason code */

static void every_code_and_arm_is_reached(void)
{
    claim_t c;
    size_t  i;

    printf("1. the twenty-two cases, and every code, arm, service state and reason\n");

    CHECK(CASE_COUNT == 22u, "chapter 02 says twenty-two cases, this file has %u",
          (unsigned)CASE_COUNT);

    claim_init(&c);

    for (i = 0u; i < CASE_COUNT; i++) {
        const case_t    *k = &CASES[i];
        claim_inputs_t   in = inputs_of(k);
        claim_decision_t d;

        /* Activation is a setting, not a per-reading input, so the case sets it there. */
        c.settings.activated = k->activated;
        d = claim_step(&c, &in, (uint32_t)(i + 1u));

        CHECK(d.code == k->want_code, "case %u (%s): code was %s, expected %s",
              (unsigned)(i + 1u), k->what,
              claim_code_name(d.code), claim_code_name(k->want_code));
        CHECK(d.arm == k->want_arm, "case %u (%s): arm was %u, expected %u",
              (unsigned)(i + 1u), k->what, (unsigned)d.arm, (unsigned)k->want_arm);

        /* After EVERY case, not at the end: a broken invariant can be transient and
         * still wrong, which is P01's reason for the same habit. */
        CHECK(claim_check_invariants(&c), "case %u (%s) broke an invariant",
              (unsigned)(i + 1u), k->what);
    }

    for (i = 0u; i < CLAIM_ARM_COUNT; i++) {
        CHECK(c.arm_taken[i] > 0u, "arm %u was never taken", (unsigned)(i + 1u));
    }
    for (i = 0u; i < CLAIM_CODE_COUNT; i++) {
        CHECK(c.code_seen[i] > 0u, "code %s was never produced",
              claim_code_name((claim_code_t)i));
    }
    for (i = 0u; i < SVC_STATE_COUNT; i++) {
        CHECK(c.service_seen[i] > 0u, "service state %s was never exercised",
              claim_service_name((claim_service_t)i));
    }
    /* The seven reasons are 1 to 7; NONE is not one of them. */
    for (i = 1u; i < SVC_REASON_COUNT; i++) {
        CHECK(c.reason_seen[i] > 0u, "service reason %s was never exercised",
              claim_svc_reason_name((claim_svc_reason_t)i));
    }

    printf("   %u cases, %u claim codes and the fall-through, %u service states, "
           "%u reasons\n",
           (unsigned)CASE_COUNT, (unsigned)CLAIM_PUBLISHED_CODES,
           (unsigned)SVC_STATE_COUNT, (unsigned)SVC_REASONS);
}

/* ------------------------------------------------- 2. a live booking beats a walk-in */

static void a_live_booking_beats_a_walk_in(void)
{
    claim_inputs_t   in;
    claim_decision_t d;

    printf("2. a live booking beats a walk-in\n");

    memset(&in, 0, sizeof(in));
    in.activated   = true;
    in.window_open = true;
    in.present     = true;
    in.service     = SVC_OK;

    d = claim_decide(&in);
    CHECK(d.code == CLAIM_BOOKED, "with a window open and somebody present the claim "
          "must be booked, was %s", claim_code_name(d.code));
    CHECK(d.code != CLAIM_WALKIN, "a passer-by must not take a booked room");
    CHECK(d.arm == 3u, "the booking arm is 3, fired %u", (unsigned)d.arm);

    /* And the mirror: the same presence with no window is a walk-in, so the test is of
     * the order and not of the inputs happening to disagree. */
    in.window_open = false;
    d = claim_decide(&in);
    CHECK(d.code == CLAIM_WALKIN, "the same presence without a window is a walk-in, "
          "was %s", claim_code_name(d.code));
}

/* ---------------------------------------------- 3. a no-show releases exactly once */

static void a_no_show_releases_exactly_once_and_says_so(void)
{
    claim_t        c;
    claim_inputs_t in;
    uint32_t       before;
    int            i;

    printf("3. a no-show emits exactly one event, and its own code\n");

    claim_init(&c);
    c.settings.activated = true;

    memset(&in, 0, sizeof(in));
    in.window_open = true;
    in.present     = false;
    in.service     = SVC_OK;

    /* Inside grace for a while. */
    for (i = 0; i < 5; i++) {
        (void)claim_step(&c, &in, (uint32_t)i);
    }
    before = c.emitted;

    /* The grace period runs out, and then nothing changes for a long time. */
    in.past_grace = true;
    for (i = 0; i < 20; i++) {
        (void)claim_step(&c, &in, (uint32_t)(100 + i));
    }

    CHECK(c.last.code == CLAIM_NO_SHOW, "the claim must be a no-show, was %s",
          claim_code_name(c.last.code));
    CHECK(c.emitted == before + 1u,
          "exactly one event for a no-show: %u before, %u after",
          (unsigned)before, (unsigned)c.emitted);
    CHECK(c.published == CLAIM_NO_SHOW,
          "the event that left must carry the no-show code, carried %s",
          claim_code_name(c.published));
    CHECK(c.last.code != CLAIM_FREE,
          "a no-show must not look like an ordinary release");
}

/* ----------------------------------------------------------- 4. grace does not emit */

static void grace_does_not_emit(void)
{
    claim_t        c;
    claim_inputs_t in;
    uint32_t       before;
    int            i;

    printf("4. grace emits nothing, however often it is the decision\n");

    claim_init(&c);
    c.settings.activated = true;

    memset(&in, 0, sizeof(in));
    in.window_open = true;
    in.service     = SVC_OK;

    before = c.emitted;
    for (i = 0; i < 200; i++) {
        claim_decision_t d = claim_step(&c, &in, (uint32_t)i);

        CHECK(d.code == CLAIM_GRACE, "reading %d of a booked empty room should be "
              "grace, was %s", i, claim_code_name(d.code));
    }

    CHECK(c.emitted == before,
          "two hundred readings inside grace must emit nothing: %u emitted",
          (unsigned)(c.emitted - before));
    CHECK(c.spool.count == 0u, "and nothing may reach the spool, %u did",
          (unsigned)c.spool.count);
    CHECK(c.code_seen[CLAIM_GRACE] == 200u, "grace was the decision %u times",
          (unsigned)c.code_seen[CLAIM_GRACE]);
}

/* ------------------------------------- 5. an unactivated unit never shows itself free */

static void an_unactivated_unit_never_shows_itself_free(void)
{
    size_t i;

    printf("5. with the flag clear, no case produces anything but invisible\n");

    for (i = 0u; i < CASE_COUNT; i++) {
        claim_t          c;
        claim_inputs_t   in = inputs_of(&CASES[i]);
        claim_decision_t d;

        claim_init(&c);
        c.settings.activated = false;          /* whatever the case asked for */

        d = claim_step(&c, &in, (uint32_t)i);
        CHECK(d.code == CLAIM_INVISIBLE,
              "case %u with the flag clear gave %s", (unsigned)(i + 1u),
              claim_code_name(d.code));
        CHECK(d.code != CLAIM_FREE && d.code != CLAIM_WALKIN,
              "case %u offered the room while uncommissioned", (unsigned)(i + 1u));
        CHECK(claim_check_invariants(&c), "case %u broke an invariant while clear",
              (unsigned)(i + 1u));
    }
}

/* ------------------------------------------- 6. the room works with the radio down */

static void the_room_works_with_the_radio_down(void)
{
    claim_t  drains;
    claim_t  down;
    size_t   i;
    bool     same = true;

    printf("6. the same decisions with the spool full as with it empty\n");

    claim_init(&drains);
    claim_init(&down);

    /* "The link is down" means nothing leaves the spool, so it fills and then discards.
     * The one on the left is drained after every step. The decisions must be identical,
     * because the cascade has no link input at all: that is the structural reason this
     * criterion holds, and the run is what shows the structure was not quietly broken. */
    for (i = 0u; i < CASE_COUNT * 40u; i++) {
        const case_t    *k = &CASES[i % CASE_COUNT];
        claim_inputs_t   in = inputs_of(k);
        claim_decision_t a;
        claim_decision_t b;

        drains.settings.activated = k->activated;
        down.settings.activated   = k->activated;

        a = claim_step(&drains, &in, (uint32_t)i);
        b = claim_step(&down, &in, (uint32_t)i);

        if (a.code != b.code || a.arm != b.arm) {
            same = false;
        }

        /* Drain the one that has a working radio. */
        drains.spool.head  = 0u;
        drains.spool.count = 0u;
        drains.spool.offered   = 0u;
        drains.spool.discarded = 0u;
        drains.emitted         = 0u;
    }

    CHECK(same, "a full spool changed a decision, so something reads the link");
    CHECK(down.spool.count == claim_spool_capacity(&down.spool),
          "with the radio down the spool should be full: %u of %u",
          (unsigned)down.spool.count, (unsigned)claim_spool_capacity(&down.spool));
    CHECK(claim_check_invariants(&down), "the invariants must hold with the radio down");
}

/* --------------------------------------- 7. the spool is bounded and the loss counted */

static void the_spool_is_bounded_and_the_loss_is_counted(void)
{
    claim_spool_t sp;
    claim_event_t ev;
    size_t        cap;
    uint32_t      i;
    const uint32_t offered = 1000u;

    printf("7. a thousand events into a bounded spool, and the arithmetic\n");

    claim_spool_init(&sp, CLAIM_SPOOL_BYTES_DEFAULT);
    cap = claim_spool_capacity(&sp);

    printf("   event %u bytes, bound %u bytes, so capacity %u events\n",
           (unsigned)claim_event_bytes(), (unsigned)CLAIM_SPOOL_BYTES_DEFAULT,
           (unsigned)cap);

    memset(&ev, 0, sizeof(ev));
    for (i = 0u; i < offered; i++) {
        ev.at_s = i;
        ev.code = (uint8_t)CLAIM_WALKIN;
        (void)claim_spool_push(&sp, &ev);
    }

    CHECK(claim_spool_count(&sp) == cap,
          "the spool must sit at its capacity: %u of %u",
          (unsigned)claim_spool_count(&sp), (unsigned)cap);
    CHECK(claim_spool_discarded(&sp) == offered - (uint32_t)cap,
          "discarded must be offered minus capacity: %u, expected %u",
          (unsigned)claim_spool_discarded(&sp), (unsigned)(offered - (uint32_t)cap));
    CHECK((uint32_t)claim_spool_count(&sp) + claim_spool_discarded(&sp) == offered,
          "retained plus discarded must equal offered: %u + %u against %u",
          (unsigned)claim_spool_count(&sp), (unsigned)claim_spool_discarded(&sp),
          (unsigned)offered);

    /* The oldest went first, so the newest is still there and the oldest retained is
     * exactly offered minus capacity. A ring that discarded the newest would pass every
     * count above and keep a record of a room that has moved on. */
    CHECK(sp.slot[sp.head].at_s == offered - (uint32_t)cap,
          "the oldest retained should be event %u, is %u",
          (unsigned)(offered - (uint32_t)cap), (unsigned)sp.slot[sp.head].at_s);
    CHECK(sp.slot[(sp.head + sp.count - 1u) % sp.cap].at_s == offered - 1u,
          "the newest retained should be event %u, is %u",
          (unsigned)(offered - 1u),
          (unsigned)sp.slot[(sp.head + sp.count - 1u) % sp.cap].at_s);

    printf("   offered %u, retained %u, discarded %u\n",
           (unsigned)offered, (unsigned)claim_spool_count(&sp),
           (unsigned)claim_spool_discarded(&sp));
}

/* ----------------------------------------------------------------------- the run */

int main(void)
{
    printf("the claim cascade, on a host, with no kernel and no board\n\n");

    every_code_and_arm_is_reached();
    a_live_booking_beats_a_walk_in();
    a_no_show_releases_exactly_once_and_says_so();
    grace_does_not_emit();
    an_unactivated_unit_never_shows_itself_free();
    the_room_works_with_the_radio_down();
    the_spool_is_bounded_and_the_loss_is_counted();

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures;
}
