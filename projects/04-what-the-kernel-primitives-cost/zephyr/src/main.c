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
#include <cmsis_core.h>
#include <zephyr/sys/util.h>
#include <zephyr/timing/timing.h>
#include <zephyr/drivers/gpio.h>

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

/* WHICH INSTRUMENT, AND WHY THERE ARE NOW TWO.
 *
 * Criterion 2 failed on Friday 9 October 2026 by a factor of five to seven against Zephyr's own
 * latency_measure suite, on two spans whose endpoints were established line by line from the
 * upstream source and do match. The residuals are the clue: ours less upstream is 1107 counts on
 * one context switch and 1171 on a semaphore wake, agreeing to 5.6 per cent. **A cost that does
 * not change when the path does sits outside the operation**, and the first suspect outside the
 * operation is the thing holding the stopwatch.
 *
 * Ours has always called k_cycle_get_32(). Upstream calls the timing API, which on this part
 * reads the cycle counter directly. So the instrument becomes selectable, both are kept, and the
 * run prints which one it used.
 *
 * KEEPING BOTH IS THE POINT, not caution. If the timing API is substituted and the old path
 * deleted, the figures will change and there will be nothing to attribute the change to. With
 * both present and named in the output, every capture says which stopwatch produced it, and the
 * k_cycle_get_32 figures already in BENCH.md stay comparable rather than becoming orphans.
 *
 * This test can fail either way and that is why it is worth a build. If the figures fall toward
 * upstream's, this project has been pricing its own measuring apparatus. If they do not move, the
 * difference is inside the brackets and the next suspect is CONFIG_POLL, which we set and
 * upstream does not, and which puts poll notification into every semaphore give.
 */
#ifndef MEASURE_USE_TIMING_API
#define MEASURE_USE_TIMING_API 1
#endif

uint32_t measure_now(void)
{
#if MEASURE_USE_TIMING_API
	return (uint32_t)timing_counter_get();
#else
	return k_cycle_get_32();
#endif
}

uint32_t measure_clock_hz(void)
{
#if defined(CONFIG_BOARD_NUCLEO_H7A3ZI_Q)
	/* Each instrument reports its own rate, because a count is only a time under the clock of
	 * the counter that produced it. Mixing the two would be the error criterion 7 exists to
	 * prevent, one level down. The 200 ms gate below checks whichever is returned. */
#if MEASURE_USE_TIMING_API
	return (uint32_t)timing_freq_get();
#else
	return (uint32_t)sys_clock_hw_cycles_per_sec();
#endif
#else
	/* NOT THIS BOARD, SO NOT A CONFIRMED CLOCK. Every capture from this build is refused by
	 * the reduction, which is the intended outcome rather than an inconvenience. */
	return 0u;
#endif
}

/* THE WALL INSTRUMENT: the one that keeps running through idle, for the one bracket that
 * contains a sleep. It is k_cycle_get_32(), driven by the system timer, and it is exactly the
 * counter this project measured everything with until Friday 9 October 2026 and then stopped
 * using for six of the seven rows because it was most of every figure. It stays for the
 * seventh because the seventh is a millisecond, the overhead is a few hundred counts, and the
 * core instrument cannot see a sleep at all. The right instrument for a row is the one that can
 * see the row. */
uint32_t measure_now_wall(void)
{
	return k_cycle_get_32();
}

uint32_t measure_clock_hz_wall(void)
{
#if defined(CONFIG_BOARD_NUCLEO_H7A3ZI_Q)
	return (uint32_t)sys_clock_hw_cycles_per_sec();
#else
	return 0u;
#endif
}

/* CRITERION 3: THE SAME OPERATION WITH A COLD INSTRUCTION CACHE.
 *
 * Worth building only because the caches turned out to be on. The run of Thursday 8 October
 * 2026 asked the control register and both are enabled, so there really are two states here
 * and the warm-up visible in every operation's first samples has something behind it.
 *
 * THE INVALIDATION SITS IMMEDIATELY BEFORE THE BRACKET OPENS, AND WHERE THAT IS DIFFERS BY
 * OPERATION. This is the correction of Friday 9 October 2026 and it is the whole lesson of
 * the first attempt, which put the call at the top of every loop and got two sound rows and
 * two worthless ones.
 *
 * Two of the four operations open their bracket on a timestamp taken in the measurer, the
 * statement after the invalidation, and those two measured 734 and 912 counts of penalty.
 * The other two open theirs on `t_handoff`, STAMPED IN THE GIVER THREAD, with a complete
 * semaphore give, a blocking take and the giver's own wake-up running untimed in between.
 * That run-up walks the same kernel code the bracket is about, so it refilled the cache the
 * invalidation had just emptied, and those two measured 172 counts and nothing at all.
 *
 * So the giver invalidates, on its last statement before it stamps. The measurer's own call
 * is removed from those two loops rather than left as well: a second invalidation earlier in
 * the iteration changes nothing and would suggest to a reader that it does.
 *
 * Outside the bracket in every case. What is measured is the operation running cold, not the
 * cost of making it cold, and those are different questions.
 *
 * The instruction cache only. Invalidating the data cache without cleaning it first would
 * discard anything dirty, and this case has no need to touch it: what a cold run is being
 * asked about is the cost of fetching the kernel's code again.
 */
static volatile bool cold_mode;

static void chill(void)
{
	if (cold_mode) {
		SCB_InvalidateICache();
		__DSB();
		__ISB();
	}
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
		/* CRITERION 3 FOR BOTH BLOCKING ROWS HAPPENS HERE, not in the measurer. The
		 * bracket opens on the stamp below, so this is the last point at which an
		 * invalidation still lands outside it and ahead of it. In the warm pass
		 * cold_mode is false and this is one test of a flag. */
		chill();
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
		chill();
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
		/* No chill() here. The giver does it, on its last statement before the stamp
		 * this bracket opens on. See the criterion 3 comment above the flag. */
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
		/* No chill() here either, for the same reason, and this row is the warning the
		 * other one is not: it showed 172 counts of penalty, a figure small enough to
		 * look like a modest real effect rather than like a broken case. */
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
		chill();
		uint32_t t0 = measure_now();

		(void)k_work_submit(&work_item);
		k_sem_take(&work_done, K_FOREVER);
		counts[i] = t_handoff - t0;
	}
}

/* THE MARKER: the one pin the witness watches, and the only output this application has.
 *
 * Named in app.overlay as PB4, CN7 pin 19, and read from there: this file does not know the
 * port or the pin, only that the overlay promised one. The run prints the controller node and
 * the pin number it was actually handed, so the capture carries the pin from the build rather
 * than from a comment, which is the same reason the build stamp exists.
 *
 * ONE EDGE PER PERIOD, ALTERNATING, NEVER A PULSE. The MCC 118 samples every 10 microseconds
 * and a pulse of two consecutive stores is tens of nanoseconds wide; the sibling firmware
 * volume tried the pulse first and the witness caught fragments of it, up to half a volt, in
 * 346 samples of 199784. A toggle leaves the level where the boundary put it until the next
 * boundary, so every crossing in either direction is a period boundary and the interval
 * between consecutive edges is one period. witness.py counts both polarities.
 *
 * THE TOGGLE SITS AT THE STAMP. run_period reads the wall counter and toggles the pin in
 * consecutive statements, so the instant the processor's instrument records and the instant
 * the witness sees are the same instant to within a store. A toggle placed anywhere else in
 * the loop would put a constant offset between the two instruments and criterion 5 would be
 * measuring that offset.
 */
static const struct gpio_dt_spec marker = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), marker_gpios);
static bool marker_ready;

/* THE BEACON, AND WHY A MEASUREMENT GETS A PREAMBLE IT DOES NOT NEED.
 *
 * The period row is 64 sleeps of a millisecond: seventy milliseconds of marker in a recording
 * that is thirty seconds long. Twice on Friday 9 October 2026 the witness recorded a flat line,
 * and the first time two clocks showed the Pi had started listening eighteen seconds after the
 * board had finished. **Asking a person to land a thirty second window on a seventy millisecond
 * event is a coordination problem, not a measurement problem**, and it can be removed rather
 * than practised.
 *
 * So the pin carries two seconds of slow square wave before anything is measured. Any recording
 * that overlaps the board's first seconds at all contains it, which makes the beacon a test of
 * the wire that needs no timing skill: beacon present means the lead reaches CH0 and the pin
 * drives it, and a flat recording after that is the lead and nothing else.
 *
 * ITS INTERVALS ARE DELIBERATELY NOTHING LIKE THE MEASUREMENT'S. 250 ms against 1.1 ms is a
 * factor of 227, so witness.py can separate the burst from the preamble by interval alone and
 * neither can be mistaken for the other. The measurement is unaffected: the beacon finishes,
 * the pin rests, and the first bracketed row starts afterwards.
 */
#define BEACON_EDGES 8u
#define BEACON_MS    250u

static void marker_beacon(void)
{
	unsigned int i;

	if (!marker_ready) {
		return;
	}
	printf("# marker beacon: %u edges %u ms apart, about %u ms of slow square wave,\n",
	       (unsigned int)BEACON_EDGES, (unsigned int)BEACON_MS,
	       (unsigned int)(BEACON_EDGES * BEACON_MS));
	printf("#   so a witness recording that overlaps this boot at all contains it\n");

	for (i = 0u; i < BEACON_EDGES; i++) {
		k_msleep((int32_t)BEACON_MS);
		(void)gpio_pin_toggle_dt(&marker);
	}
	/* Left low, so the measurement burst starts from the same level every run. */
	(void)gpio_pin_set_dt(&marker, 0);
}

/* THE RESTING BEACON, WHICH NEVER RETURNS, AND WHY IT IS THE ONE THAT MATTERS.
 *
 * The beacon above still asks somebody to start a recording near a reset. This one does not.
 * After every row has been measured and printed, the pin toggles at a quarter of a second for
 * as long as the board is powered, so a witness recording started at ANY later moment contains
 * it. **The wire can then be tested with no coordination whatsoever**: flash, reset, walk away,
 * record whenever, and either the edges are there or the lead is not on the pin.
 *
 * Twice on Friday 9 October 2026 a flat recording left two live causes, a mistimed window and
 * a wrong lead, and no way to tell them apart. This separates them permanently and costs
 * nothing, because the application had finished its work and was returning from main anyway.
 *
 * Its interval is the beacon's, far above witness.py's burst threshold, so select_burst()
 * excludes it from the measurement exactly as it excludes the preamble.
 */
static void marker_rest(void)
{
	if (!marker_ready) {
		printf("\n# no marker, so no resting beacon. The pin could not be configured.\n");
		return;
	}
	printf("\n# resting beacon: the marker now toggles every %u ms for as long as this\n",
	       (unsigned int)BEACON_MS);
	printf("#   board is powered, so a witness recording started at any later moment\n");
	printf("#   contains it. Edges here mean the lead reaches CH0; a flat recording\n");
	printf("#   taken now means it does not, and no timing is involved either way.\n");

	for (;;) {
		k_msleep((int32_t)BEACON_MS);
		(void)gpio_pin_toggle_dt(&marker);
	}
}

/* THE PERIOD, which is the one row the external witness can also see, and the only row where
 * two instruments are meant to agree.
 *
 * IT IS ALSO THE ONE ROW WHOSE BRACKET CONTAINS A SLEEP, which makes the two instruments in this
 * file **not interchangeable here**. Established Friday 9 October 2026: the timing API's counter
 * counts core cycles and stops when the core stops, so across a k_msleep it reads the handful of
 * microseconds the wake-up took rather than the millisecond that passed. A counter that stops
 * during sleep cannot measure a sleep.
 *
 * That is not a defect in either counter. It is the instrument being chosen per operation rather
 * than per build, which is what the other six rows had never needed. The bracketed primitives
 * contain no idle at all, so a core cycle counter suits them and is what upstream uses; this row
 * wants the system timer, which keeps running through idle.
 *
 * For a few hours on Friday 9 October 2026 this row was refused outright, because the capture
 * carried one `clock_hz` line and a row measured with one counter and labelled with the other's
 * rate would be the mislabelling criterion 7 exists to catch, one level down. The emit now
 * carries an `instrument` line and the rate that goes with it, from one lookup in the core, so
 * the row is back on the counter that can see it. The reduction refuses a period capture
 * labelled `core`, so the two halves hold each other to it.
 */
static int run_period(uint32_t *counts, size_t n)
{
	uint32_t previous;
	size_t i;

	/* NO MARKER, NO ROW. A period without the witness able to see it is a number this
	 * project already has from Thursday 8 October 2026; what this row exists for today is
	 * the agreement of two instruments, and a capture the witness could not have seen
	 * would be reduced against nothing. Refused, with the reason on the console. */
	if (!marker_ready) {
		printf("# the marker pin from app.overlay could not be configured, so the\n");
		printf("# witness has nothing to watch and the period row is withheld\n");
		return -ENODEV;
	}

	k_msleep(1);
	previous = measure_now_wall();
	(void)gpio_pin_toggle_dt(&marker);

	for (i = 0u; i < n; i++) {
		k_msleep(1);
		uint32_t now = measure_now_wall();

		/* The stamp and the edge are consecutive statements on purpose: see the
		 * marker comment above. n+1 edges for n periods, so the witness's intervals
		 * line up one to one with a_counts. */
		(void)gpio_pin_toggle_dt(&marker);
		counts[i] = now - previous;
		previous = now;
	}
	return 0;
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
		return run_period(counts, n);
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

	/* A BUSY INTERVAL, NOT A SLEEP, AND THE REASON IS A REFUSAL THIS GATE PRODUCED.
	 *
	 * At 09:20 on Friday 9 October 2026 the first run on the timing API refused: 4260 counts
	 * against a predicted 56000000. 4260 counts at the 280 MHz the instrument reports is 15.2
	 * microseconds of activity inside a 200 millisecond sleep, and with tickless idle a
	 * k_msleep(200) is one wake-up and almost nothing else. **The counter counts core cycles
	 * and stops when the core stops.** Nothing was wrong with it. The gate was asking it to
	 * time an interval during which it is switched off.
	 *
	 * So the window is spun rather than slept, which keeps the core out of WFI, and its length
	 * is set by k_uptime_get(). That matters: uptime is the kernel's millisecond clock off the
	 * system timer, so it is **neither of the two cycle counters**. Had the window been set by
	 * k_cycle_get_32() instead, the check would be self-referential whenever that is also the
	 * instrument, and a gate that cannot fail has shown nothing.
	 *
	 * This also tests its own diagnosis. If the counter is a core cycle counter that merely
	 * stops in idle, a busy window puts it at 280 MHz and the gate passes. If it does not, the
	 * explanation above is wrong and the refusal was about something else.
	 */
	int64_t up0 = k_uptime_get();

	t0 = measure_now();
	while ((k_uptime_get() - up0) < (int64_t)CLOCK_CHECK_MS) {
		/* Spinning on purpose. An idle core is a stopped instrument. */
	}
	elapsed = measure_now() - t0;

	printf("# clock check: %u ms of sleep advanced the counter by %u, and %u Hz\n",
	       (unsigned int)CLOCK_CHECK_MS, (unsigned int)elapsed, (unsigned int)hz);
	printf("#   predicts %u, so the count is %u per cent of the prediction\n",
	       (unsigned int)predicted,
	       (unsigned int)(((uint64_t)elapsed * 100u) / predicted));

	/* Within a quarter. Generous because this does not measure the window, it asks whether the
	 * declared rate and the observed rate are the same order of thing. Being wrong by a factor
	 * is what it has to catch, and on Friday 9 October 2026 it caught one. */
	return elapsed > (predicted - predicted / 4u) && elapsed < (predicted + predicted / 4u);
}

/* THE SAME GATE FOR THE WALL INSTRUMENT, AND THIS ONE SLEEPS ON PURPOSE.
 *
 * The core gate above spins because its counter stops in idle. The wall instrument exists
 * precisely because it does not, so the honest window for it is a sleep: if it cannot time a
 * k_msleep to within a quarter, it cannot time the period row either, and that row is the only
 * reason it is here. This is the gate the file had until Friday 9 October 2026, back on the
 * counter it was right for.
 *
 * Two gates with two windows is not duplication. Each checks the one property its counter is
 * relied on for, and a single gate that passed both would have to spin, which would never
 * discover that the wall counter had stopped counting in idle. */
static bool wall_clock_rate_agrees(void)
{
	uint32_t hz = measure_clock_hz_wall();
	uint32_t predicted, elapsed, t0;

	if (hz == 0u) {
		return false;
	}

	predicted = (hz / 1000u) * CLOCK_CHECK_MS;
	t0 = measure_now_wall();
	k_msleep((int32_t)CLOCK_CHECK_MS);
	elapsed = measure_now_wall() - t0;

	printf("# wall clock check: %u ms of SLEEP advanced the wall counter by %u, and %u Hz\n",
	       (unsigned int)CLOCK_CHECK_MS, (unsigned int)elapsed, (unsigned int)hz);
	printf("#   predicts %u, so the count is %u per cent of the prediction\n",
	       (unsigned int)predicted,
	       (unsigned int)(((uint64_t)elapsed * 100u) / predicted));

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

/* ARE THE CACHES ON? The question behind every figure this project has produced.
 *
 * A single context switch measured about 1300 counts, which is 4.6 us at 280 MHz and slow
 * for a Cortex-M7. The standing suspicion has been the caches and the flash wait states
 * rather than the kernel, and two builds differing only in code that nothing executes moved
 * the figures by about one per cent each, which is what instruction placement does.
 *
 * This asks the hardware rather than the configuration. Kconfig says what the build asked
 * for; the control register says what the processor is actually doing, and those are two
 * different claims. If they disagree, the disagreement is the finding.
 *
 * It also decides whether criterion 3 is worth building. Warm against cold is a comparison
 * between two cache states, and with no cache there is only one state and nothing to
 * compare, so a run showing both off would retire that criterion rather than fail it.
 */
static void report_caches(void)
{
	printf("# caches, as the build asked and as the hardware reports\n");
	printf("#   the build asked for CONFIG_ICACHE %s and CONFIG_DCACHE %s\n",
	       IS_ENABLED(CONFIG_ICACHE) ? "y" : "n",
	       IS_ENABLED(CONFIG_DCACHE) ? "y" : "n");
	printf("#   the control register says instruction cache %s, data cache %s\n",
	       (SCB->CCR & SCB_CCR_IC_Msk) ? "ON" : "OFF",
	       (SCB->CCR & SCB_CCR_DC_Msk) ? "ON" : "OFF");
}

/* THE SETTINGS THAT BEAR ON THE COMPARISON, ASKED OF THE BUILD RATHER THAN ASSUMED.
 *
 * Added Friday 9 October 2026 after a default went unread for the life of the project.
 * CONFIG_TIMESLICING defaults to y and CONFIG_TIMESLICE_SIZE to 20 ms, so every figure taken
 * before today carried slice bookkeeping in the scheduler path that Zephyr's own
 * latency_measure suite does not, because that suite sets CONFIG_TIMESLICING=n.
 *
 * The lesson is the one report_caches() already carries: **the build is the authority on what
 * was built, and a default is a claim nobody has checked.** A Kconfig default read out of the
 * upstream tree is a claim about a version; this line is a statement about the binary that is
 * running. Criterion 2 compares two configurations, so each has to state its own.
 */
static void report_config(void)
{
	printf("# configuration that bears on the comparison with the upstream suite\n");
	printf("#   CONFIG_TIMESLICING %s", IS_ENABLED(CONFIG_TIMESLICING) ? "y" : "n");
#ifdef CONFIG_TIMESLICE_SIZE
	printf(", slice %d ms", (int)CONFIG_TIMESLICE_SIZE);
#endif
	printf("\n");
	printf("#   CONFIG_ASSERT %s, CONFIG_POLL %s, and upstream sets both n and y\n",
	       IS_ENABLED(CONFIG_ASSERT) ? "y" : "n",
	       IS_ENABLED(CONFIG_POLL) ? "y" : "n");
	/* The guard region reprogrammed on every thread switch, and the two settings it needs.
	 * Printed because this is the one difference from upstream that both sides state
	 * explicitly and in opposite directions, so neither default can be assumed. */
	printf("#   CONFIG_ARM_MPU %s, CONFIG_HW_STACK_PROTECTION %s, upstream sets the latter n\n",
	       IS_ENABLED(CONFIG_ARM_MPU) ? "y" : "n",
	       IS_ENABLED(CONFIG_HW_STACK_PROTECTION) ? "y" : "n");
	printf("#   CONFIG_PM %s and CONFIG_FPU_SHARING %s, both expected n on both sides\n",
	       IS_ENABLED(CONFIG_PM) ? "y" : "n",
	       IS_ENABLED(CONFIG_FPU_SHARING) ? "y" : "n");
	/* THE STOPWATCHES NAME THEMSELVES, for the same reason the application does. Two captures
	 * with different instruments and no line saying which are two numbers nobody can
	 * reconcile. Each capture below also carries its own `instrument` line. */
	printf("#   core instrument, six rows: %s\n",
	       MEASURE_USE_TIMING_API ? "the timing API, as upstream uses"
				      : "k_cycle_get_32, as every run before 9 October 2026");
	printf("#   wall instrument, the period row: k_cycle_get_32, which runs through a sleep\n");
	/* THE PIN, FROM THE BUILD. The controller is printed as its devicetree node, which on this
	 * part carries the port's register base: gpio@58020400 is GPIOB in RM0455, and the pin is
	 * the bit. docs/WIRING.md makes that translation once, with the citation; the line below
	 * is what a reader checks it against. */
	printf("#   marker for the witness: pin %u on %s, %s\n",
	       (unsigned int)marker.pin,
	       DT_NODE_FULL_NAME(DT_GPIO_CTLR(DT_PATH(zephyr_user), marker_gpios)),
	       marker_ready ? "configured as an output, one toggle per period"
			    : "NOT CONFIGURED, so the period row will be withheld");
}

int main(void)
{
	static uint32_t counts[SAMPLES];
	uint32_t overhead = 0u;
	uint32_t smallest = 0xFFFFFFFFu;
	uint32_t warm_min[MEASURE_OP_COUNT];
	uint32_t cold_min_of[4];
	int op;

	k_thread_create(&partner_thread, partner_stack, STACK_SIZE, partner_fn,
			NULL, NULL, NULL, PRIO_PARTNER, 0, K_NO_WAIT);
	k_thread_suspend(&partner_thread);
	k_thread_create(&giver_thread, giver_stack, STACK_SIZE, giver_fn,
			NULL, NULL, NULL, PRIO_GIVER, 0, K_NO_WAIT);
	k_thread_priority_set(k_current_get(), PRIO_MEASURER);

#if MEASURE_USE_TIMING_API
	/* Before anything asks the instrument for a rate or a count. timing_freq_get() has nothing
	 * to report until this has run, and a zero rate would be refused by the gate below, which
	 * is the right failure but an uninformative one. */
	timing_init();
	timing_start();
#endif

	/* Before any row runs, so that report_config() can say what it found and run_period()
	 * can refuse if there is nothing to toggle. Driven low to start, matching the sibling
	 * volume's marker_init, so the first toggle is the first rising edge. */
	marker_ready = gpio_is_ready_dt(&marker) &&
		       gpio_pin_configure_dt(&marker, GPIO_OUTPUT_INACTIVE) == 0;

	printf("# p04 under zephyr\n");
	/* THE BUILD STAMP, AND IT EXISTS BECAUSE THE NAME ABOVE WAS NOT ENOUGH.
	 *
	 * The shared-board rule says each application names itself, so the first line of a run
	 * tells you whether the right project is on the part. On Friday 9 October 2026 that check
	 * passed and was still wrong: the line read `p04 under zephyr`, correctly, and the image
	 * was the previous commit's. A rebuild had been skipped, the name cannot see a commit, and
	 * the figures that came back were an exact repeat that looked like a reproduction.
	 *
	 * So the run states when it was compiled. __DATE__ and __TIME__ are the two macros whose
	 * whole purpose is to differ between builds, they need nothing from the build system, and
	 * a stamp older than the last edit is the signal. A reproduction and a stale flash produce
	 * the same numbers, and only this line separates them. */
	printf("# built %s %s, and a stamp older than your last edit means a stale flash\n",
	       __DATE__, __TIME__);
	if (measure_clock_hz() == 0u || measure_clock_hz_wall() == 0u) {
		printf("# THE CLOCK IS UNCONFIRMED FOR THIS BUILD. Every capture below carries\n");
		printf("# clock_hz 0 and the reduction refuses it. That is the point: these\n");
		printf("# counts are not times and nothing should be able to publish them.\n");
	} else if (!clock_rate_agrees() || !wall_clock_rate_agrees()) {
		printf("#\n# REFUSED. The counter does not advance at the rate the kernel\n");
		printf("# declares, so every count below would be scaled by an unknown\n");
		printf("# factor while looking entirely reasonable. Nothing is emitted.\n");
		return 1;
	}

	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		warm_min[op] = 0xFFFFFFFFu;
	}

	report_caches();
	report_config();
	marker_beacon();
	overhead = instrument_cost();

	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		int rc = measure_run((measure_op_t)op, counts, SAMPLES);

		printf("\n");
		if (rc != 0) {
			printf("# %s: not measured in this build, %d\n",
			       measure_op_name((measure_op_t)op), rc);
			continue;
		}
		(void)measure_emit((measure_op_t)op, counts, SAMPLES);

		/* One pass, two minima. These were two nested loops until Friday 9 October
		 * 2026, running 4096 iterations for 64 samples. A minimum is idempotent so no
		 * figure was ever wrong, and it is repaired because a misplaced brace that
		 * happens not to matter is still a misplaced brace. */
		for (size_t k = 0u; k < SAMPLES; k++) {
			/* Criterion 1 prices the CORE instrument against the smallest thing the core
			 * instrument measured. A wall-instrument row does not enter that minimum:
			 * it is a different counter, and a millisecond besides. */
			if (measure_op_instrument((measure_op_t)op) == MEASURE_INSTRUMENT_CORE &&
			    counts[k] < smallest) {
				smallest = counts[k];
			}
			if (counts[k] < warm_min[op]) {
				warm_min[op] = counts[k];
			}
		}
	}

	printf("\n# %d operations attempted\n", (int)MEASURE_OP_COUNT);

	/* CRITERION 3. The first four operations again, cold. The period row is a sleep and
	 * the mutex rows take long enough that a cache state would be lost inside them, so
	 * neither is repeated: a cold measurement of a millisecond would be a cold first
	 * microsecond and a warm rest, which is not the comparison the criterion asks for. */
	cold_mode = true;
	printf("\n# criterion 3: the same four operations with the instruction cache\n");
	printf("#   invalidated immediately before each bracket opens, outside it\n");

	/* THE DISTRIBUTION, NOT ONLY THE FLOOR, and the reason is a question the first version of
	 * this pass could not answer. On Friday 9 October 2026 one row came out four counts lower
	 * cold than warm, and whether 1468 was the floor of a tight distribution or one low sample
	 * in a scattered one could not be told, because only the minimum was printed. A minimum
	 * was enough for the instrument's own cost, where the floor is the whole point. It is not
	 * enough for a comparison.
	 *
	 * Printed as a comment rather than through measure_emit(). The capture format has no field
	 * for the cache state, so a second capture per operation would be indistinguishable from
	 * the first except by its position in the log, and a number whose conditions cannot be read
	 * off the record should not be in the record. Adding that field changes the host parser and
	 * its suite, and it waits until a cold row needs to go through the reduction.
	 *
	 * The runs all happen before the table is printed, so that the table arrives in one piece
	 * with the count dumps above it rather than interleaved through it. */
	for (op = 0; op < 4; op++) {
		size_t k;

		cold_min_of[op] = 0xFFFFFFFFu;
		if (measure_run((measure_op_t)op, counts, SAMPLES) != 0) {
			continue;
		}
		for (k = 0u; k < SAMPLES; k++) {
			if (counts[k] < cold_min_of[op]) {
				cold_min_of[op] = counts[k];
			}
		}

		printf("#\n#   cold counts, %s\n#  ", measure_op_name((measure_op_t)op));
		for (k = 0u; k < SAMPLES; k++) {
			printf(" %u", (unsigned int)counts[k]);
		}
		printf("\n");
	}
	cold_mode = false;

	printf("#\n#   minimum of 64, because an operation has a floor and no ceiling\n");
	printf("#   the penalty in counts is the figure to quote, not the percentage:\n");
	printf("#   a percentage carries whatever the baseline happens to be\n");
	printf("#\n");
	printf("#   operation                                        warm    cold  penalty\n");

	for (op = 0; op < 4; op++) {
		if (cold_min_of[op] == 0xFFFFFFFFu) {
			continue;
		}
		if (cold_min_of[op] >= warm_min[op]) {
			printf("#   %-46s %6u  %6u  %7u  cold is %u per cent higher\n",
			       measure_op_name((measure_op_t)op),
			       (unsigned int)warm_min[op], (unsigned int)cold_min_of[op],
			       (unsigned int)(cold_min_of[op] - warm_min[op]),
			       (unsigned int)(((cold_min_of[op] - warm_min[op]) * 100u)
					      / warm_min[op]));
		} else {
			/* NOT AN IMPOSSIBILITY, AND WORTH PRINTING RATHER THAN HIDING. This fired
			 * on Friday 9 October 2026 for block-on-one and it was right to: the
			 * invalidation was in the measurer and that row's bracket opens on a stamp
			 * taken in the giver, after a give and a block had already rewarmed the
			 * path. The flag found a defect in the case, which is what it is for. */
			printf("#   %-46s %6u  %6u        0  COLD IS LOWER, which should not happen\n",
			       measure_op_name((measure_op_t)op),
			       (unsigned int)warm_min[op], (unsigned int)cold_min_of[op]);
		}
	}

	if (overhead > 0u && smallest != 0xFFFFFFFFu) {
		printf("# criterion 1: the instrument costs %u counts and the smallest\n",
		       (unsigned int)overhead);
		printf("#   thing it measured was %u, a ratio of %u. The criterion asks\n",
		       (unsigned int)smallest, (unsigned int)(smallest / overhead));
		printf("#   for at least ten, so this run %s\n",
		       (smallest / overhead) >= 10u ? "meets it" : "does not");
	}

	/* Everything above is printed before this is reached, so a console capture is complete
	 * whether or not anybody is watching the pin. This does not return. */
	marker_rest();
	return 0;
}
