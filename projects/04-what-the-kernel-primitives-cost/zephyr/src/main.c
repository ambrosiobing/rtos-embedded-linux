/* projects/04-what-the-kernel-primitives-cost/zephyr/src/main.c: the Zephyr port and adapter.
 *
 * TWO HALVES OF THE CONTRACT LIVE HERE and the core next door has neither.
 *
 *   the port     measure_now() and measure_clock_hz()
 *   the adapter  measure_run(), which performs one operation n times
 *
 * THE PORT REFUSES TO REPORT A CLOCK IT HAS NOT CONFIRMED. measure_clock_hz() returns the
 * board's rate only when this is built for the board whose clock tree has been checked, and
 * zero otherwise. A zero travels into the capture as `clock_hz 0`, which the reduction refuses.
 *
 * That is deliberate and it is the whole reason the check is here rather than in a comment. A
 * host or simulator build of this application still runs, still brackets regions and still
 * prints counts, and every one of those counts is meaningless as a time. Rather than trusting
 * whoever reads the output to remember that, the build makes the capture unusable. **A number
 * that cannot be a measurement should not be publishable by accident.**
 *
 * WHAT EACH BRACKET ACTUALLY CONTAINS is written beside each operation, because a timing number
 * carries no sign of being about the wrong thing. "A context switch" is not a measurement until
 * somebody says whether it includes the trip back.
 */
#include "../../adapter/measure.h"
#include "mutex_case.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <stdio.h>

/* Lower numbers are higher priority. The measurer sits above its partner's helpers so that a
 * give or a submit returns before the other side runs, which is what makes the bracket contain
 * the wake-up rather than the hand-off. */
#define PRIO_MEASURER 5
#define PRIO_PARTNER  5   /* equal, for the yield case */
#define PRIO_GIVER    7   /* lower, so the measurer is blocked before the give happens */

#define STACK_SIZE 1024
#define SAMPLES    64u

K_THREAD_STACK_DEFINE(partner_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(giver_stack, STACK_SIZE);
static struct k_thread partner_thread;
static struct k_thread giver_thread;

static K_SEM_DEFINE(go, 0, 1);        /* the measurer releases the giver */
static K_SEM_DEFINE(signalled, 0, 1); /* what the measurer blocks on */
static K_SEM_DEFINE(second, 0, 1);    /* a second object, for the several-objects case */
static K_SEM_DEFINE(work_done, 0, 1);

static volatile uint32_t t_handoff; /* written by the other side, read by the measurer */

/* ---- the port ---------------------------------------------------------------------------- */

uint32_t measure_now(void)
{
	return k_cycle_get_32();
}

uint32_t measure_clock_hz(void)
{
#if defined(CONFIG_BOARD_NUCLEO_H7A3ZI_Q)
	return (uint32_t)sys_clock_hw_cycles_per_sec();
#else
	/* NOT THIS BOARD, SO NOT A CONFIRMED CLOCK. Every capture from this build is refused by
	 * the reduction, which is the intended outcome rather than an inconvenience. */
	return 0u;
#endif
}

/* ---- the partner and giver threads ------------------------------------------------------- */

static void partner_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		k_yield();
	}
}

static void giver_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		k_sem_take(&go, K_FOREVER);
		/* The last thing before the give, so the bracket holds the wake-up and not the
		 * giver's own run-up. */
		t_handoff = measure_now();
		k_sem_give(&signalled);
	}
}

static void work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	t_handoff = measure_now();
	k_sem_give(&work_done);
}

static K_WORK_DEFINE(work_item, work_fn);

/* ---- the operations ---------------------------------------------------------------------- */

/* A FULL ROUND TRIP, not one switch. The measurer yields, the equal-priority partner runs and
 * yields back, and the bracket closes. Reporting this as "a context switch" would halve a
 * number nobody could reproduce, so the name says what it contains. */
static void run_yield(uint32_t *counts, size_t n)
{
	size_t i;

	/* THE PARTNER RUNS ONLY HERE, AND THAT IS NOT TIDINESS. On Thursday 8 October 2026
	 * the first run on the board printed this operation and then stopped dead. The
	 * partner spins at the measurer's own priority, so while it was running the
	 * lower-priority giver could never be scheduled, and the next operation blocked
	 * on a signal that could not arrive.
	 *
	 * An always-ready thread at the measurer's priority is exactly what the yield case
	 * needs and exactly what every other case cannot survive. */
	k_thread_resume(&partner_thread);

	for (i = 0u; i < n; i++) {
		uint32_t t0 = measure_now();

		k_yield();
		counts[i] = measure_now() - t0;
	}

	k_thread_suspend(&partner_thread);
}

/* FROM THE GIVE TO THE RESUMPTION. The giver is lower priority, so releasing it does not run
 * it; the measurer then blocks, the giver runs, stamps and gives, and the measurer preempts it
 * immediately. The bracket therefore contains the wake-up and not the hand-off. */
static void run_block_one(uint32_t *counts, size_t n)
{
	size_t i;

	for (i = 0u; i < n; i++) {
		k_sem_give(&go);
		k_sem_take(&signalled, K_FOREVER);
		counts[i] = measure_now() - t_handoff;
	}
}

/* THE SAME EVENT, WAITED FOR DIFFERENTLY. Identical sequencing to the case above, so that the
 * difference between the two numbers is the waiting construct and nothing else. */
static void run_block_several(uint32_t *counts, size_t n)
{
	struct k_poll_event events[2];
	size_t i;

	k_poll_event_init(&events[0], K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
			  &signalled);
	k_poll_event_init(&events[1], K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
			  &second);

	for (i = 0u; i < n; i++) {
		k_sem_give(&go);
		(void)k_poll(events, (int)ARRAY_SIZE(events), K_FOREVER);
		counts[i] = measure_now() - t_handoff;
		k_sem_take(&signalled, K_NO_WAIT);
		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
	}
}

/* FROM THE SUBMIT TO THE HANDLER RUNNING. This is the number a chapter needs when it decides
 * whether to hand an interrupt's tail to a queue, and it is not the cost of the submit call,
 * which is the figure that usually gets quoted. */
static void run_hand_to_queue(uint32_t *counts, size_t n)
{
	size_t i;

	for (i = 0u; i < n; i++) {
		uint32_t t0 = measure_now();

		(void)k_work_submit(&work_item);
		k_sem_take(&work_done, K_FOREVER);
		counts[i] = t_handoff - t0;
	}
}

/* THE PERIOD, which is the one row the external witness can also see, and the only row where
 * two instruments are meant to agree. */
static void run_period(uint32_t *counts, size_t n)
{
	uint32_t previous;
	size_t i;

	k_msleep(1);
	previous = measure_now();

	for (i = 0u; i < n; i++) {
		k_msleep(1);
		uint32_t now = measure_now();

		counts[i] = now - previous;
		previous = now;
	}
}

int measure_run(measure_op_t op, uint32_t *counts, size_t n)
{
	if (counts == NULL || n == 0u) {
		return -EINVAL;
	}

	switch (op) {
	case MEASURE_OP_YIELD_EQUAL:
		run_yield(counts, n);
		return 0;
	case MEASURE_OP_BLOCK_ONE:
		run_block_one(counts, n);
		return 0;
	case MEASURE_OP_BLOCK_SEVERAL:
		run_block_several(counts, n);
		return 0;
	case MEASURE_OP_HAND_TO_QUEUE:
		run_hand_to_queue(counts, n);
		return 0;
	case MEASURE_OP_PERIOD:
		run_period(counts, n);
		return 0;
	case MEASURE_OP_MUTEX_INHERIT_ON:
		mutex_case_run(true, counts, n);
		return 0;
	case MEASURE_OP_MUTEX_INHERIT_OFF:
		mutex_case_run(false, counts, n);
		return 0;
	default:
		return -EINVAL;
	}
}

/* ---- the application ---------------------------------------------------------------------- */

/* IS THE DECLARED RATE THE RATE THIS COUNTER ACTUALLY ADVANCES AT?
 *
 * measure_clock_hz() reports what the kernel says the hardware cycle rate is and measure_now()
 * reads a counter. Those are two different claims and nothing had ever checked that they are
 * about the same thing. If the counter advances at some other rate, every figure in the table
 * is scaled by an unknown factor and still looks entirely reasonable.
 *
 * So bracket a sleep of a known length and compare the count against what the declared rate
 * predicts. It is the design's own rule turned on the instrument rather than on the subject,
 * and it runs before any operation, because a mis-scaled table is worse than no table.
 *
 * The tolerance is generous on purpose. A sleep is not a precise interval and this does not
 * measure the sleep; it asks whether the two claims are about the same order of thing. Two per
 * cent would fail on scheduling noise. Being wrong by a factor is what it has to catch.
 */
#define CLOCK_CHECK_MS 200u

static bool clock_rate_agrees(void)
{
	uint32_t hz = measure_clock_hz();
	uint32_t predicted, elapsed, t0;

	if (hz == 0u) {
		return false;
	}

	predicted = (hz / 1000u) * CLOCK_CHECK_MS;
	t0 = measure_now();
	k_msleep((int32_t)CLOCK_CHECK_MS);
	elapsed = measure_now() - t0;

	printf("# clock check: %u ms of sleep advanced the counter by %u, and %u Hz\n",
	       (unsigned int)CLOCK_CHECK_MS, (unsigned int)elapsed, (unsigned int)hz);
	printf("#   predicts %u, so the count is %u per cent of the prediction\n",
	       (unsigned int)predicted,
	       (unsigned int)(((uint64_t)elapsed * 100u) / predicted));

	/* Within a quarter, which catches a factor and tolerates a sleep. */
	return elapsed > (predicted - predicted / 4u) && elapsed < (predicted + predicted / 4u);
}

/* CRITERION 1: THE INSTRUMENT HAS TO BE CHEAPER THAN EVERYTHING IT MEASURES.
 *
 * Two reads of the counter sit inside every bracket in this file, and until now their cost
 * had never been measured. The criterion asks for at least an order of magnitude between
 * the instrument and the smallest thing it reports, because below that the table is partly
 * measuring the measurement.
 *
 * This is an empty bracket: two reads and nothing between them. It is reported beside the
 * clock check rather than as a row of the results table, because it is a property of the
 * instrument and not of the kernel, and a table row would invite somebody to compare it
 * with a primitive as though they were the same kind of thing.
 *
 * The MINIMUM is the figure that matters here, not the median. An empty bracket has a
 * floor and no ceiling: anything above the floor is interference, and the floor is what
 * the instrument actually costs.
 */
static uint32_t instrument_cost(void)
{
	uint32_t best = 0xFFFFFFFFu;
	uint32_t worst = 0u;
	unsigned int i;

	for (i = 0u; i < 256u; i++) {
		uint32_t t0 = measure_now();
		uint32_t d = measure_now() - t0;

		if (d < best) {
			best = d;
		}
		if (d > worst) {
			worst = d;
		}
	}

	printf("# instrument: an empty bracket costs %u counts at best, %u at worst\n",
	       (unsigned int)best, (unsigned int)worst);
	return best;
}

int main(void)
{
	static uint32_t counts[SAMPLES];
	uint32_t overhead = 0u;
	uint32_t smallest = 0xFFFFFFFFu;
	int op;

	k_thread_create(&partner_thread, partner_stack, STACK_SIZE, partner_fn,
			NULL, NULL, NULL, PRIO_PARTNER, 0, K_NO_WAIT);
	k_thread_suspend(&partner_thread);
	k_thread_create(&giver_thread, giver_stack, STACK_SIZE, giver_fn,
			NULL, NULL, NULL, PRIO_GIVER, 0, K_NO_WAIT);
	k_thread_priority_set(k_current_get(), PRIO_MEASURER);

	printf("# p04 under zephyr\n");
	if (measure_clock_hz() == 0u) {
		printf("# THE CLOCK IS UNCONFIRMED FOR THIS BUILD. Every capture below carries\n");
		printf("# clock_hz 0 and the reduction refuses it. That is the point: these\n");
		printf("# counts are not times and nothing should be able to publish them.\n");
	} else if (!clock_rate_agrees()) {
		printf("#\n# REFUSED. The counter does not advance at the rate the kernel\n");
		printf("# declares, so every count below would be scaled by an unknown\n");
		printf("# factor while looking entirely reasonable. Nothing is emitted.\n");
		return 1;
	}

	overhead = instrument_cost();

	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		int rc = measure_run((measure_op_t)op, counts, SAMPLES);

		printf("\n");
		if (rc != 0) {
			printf("# %s: not available under this kernel yet, %d\n",
			       measure_op_name((measure_op_t)op), rc);
			continue;
		}
		(void)measure_emit((measure_op_t)op, counts, SAMPLES);

		for (size_t k = 0u; k < SAMPLES; k++) {
			if (counts[k] < smallest) {
				smallest = counts[k];
			}
		}
	}

	printf("\n# %d operations attempted\n", (int)MEASURE_OP_COUNT);

	if (overhead > 0u && smallest != 0xFFFFFFFFu) {
		printf("# criterion 1: the instrument costs %u counts and the smallest\n",
		       (unsigned int)overhead);
		printf("#   thing it measured was %u, a ratio of %u. The criterion asks\n",
		       (unsigned int)smallest, (unsigned int)(smallest / overhead));
		printf("#   for at least ten, so this run %s\n",
		       (smallest / overhead) >= 10u ? "meets it" : "does not");
	}
	return 0;
}
