/* projects/04-what-the-kernel-primitives-cost/adapter/campaign.h: one run, against any kernel.
 *
 * WHY THIS EXISTS. The gates, the instrument's own cost, the warm pass, the cold pass,
 * criterion 1 and the verdicts lived in the Zephyr main until Saturday 17 October 2026, and a
 * FreeRTOS board main would have carried a second copy of three hundred lines whose only
 * kernel-specific content was how to sleep and which pin to toggle. Two copies of one
 * orchestration are two chances to orchestrate two different measurements under one name,
 * and nothing inside either would notice. P02 met the same shape and answered it with
 * phases.c: **one test passes against each kernel**, rather than each kernel passing a test
 * of its own.
 *
 * So the orchestration is here, in the core, which knows no kernel. What a kernel has to
 * supply is small and is passed in as hooks: how to report what it was built with, how to
 * spin and how to sleep for the two clock gates, how to make the cache cold, and how to drive
 * the marker the witness watches. A hook left NULL is a capability the build does not have,
 * and the campaign says so rather than pretending.
 *
 * THE PROOF THAT THE LIFT CHANGED NOTHING is the board: the Zephyr figures after it must sit
 * within the two per cent build bound of the figures before it. A refactor of a measurement
 * harness that cannot be checked against the measurement is not a refactor, it is a rewrite.
 */
#ifndef MEASURE_CAMPAIGN_H
#define MEASURE_CAMPAIGN_H

#include <stdbool.h>

/* Sixty-four, as every run since Thursday 8 October 2026. Exported so that a kernel's marker
 * loop toggles the same count the period row brackets, and a static assertion in each main
 * holds the two together. */
#define MEASURE_SAMPLES 64u

typedef struct {
	/* Printed on the first line, after "p04 under". "zephyr", "freertos", "freertos, on a
	 * host". A capture is identified by this and by the build stamp below, and the stamp
	 * exists because the name alone could not see a commit: on Friday 9 October 2026 a
	 * stale flash read as a reproduction until the stamp was added. */
	const char *kernel;
	const char *built;   /* __DATE__ " " __TIME__, evaluated in the kernel's own main */

	/* Kernel and board specifics worth a reader's eye before any figure: which caches the
	 * hardware says are on, which configuration the build was compiled with, which pin the
	 * marker drives. May be NULL. */
	void (*report)(void);

	/* The two clock gates. Each returns true when its counter advanced at the declared rate
	 * over a window of known length, and each is kernel-specific because the window is:
	 * the core gate SPINS, because its counter stops when the core idles, and the wall gate
	 * SLEEPS, because its counter exists to run through idle. NULL means the port declares
	 * no confirmed clock, and the campaign takes the unconfirmed path, emitting captures
	 * the reduction will refuse. */
	bool (*core_clock_agrees)(void);
	bool (*wall_clock_agrees)(void);

	/* Criterion 3. Makes every subsequent bracket open with the instruction cache
	 * invalidated, or stops doing so. NULL means there is no cache to invalidate, which is
	 * a host, and the cold pass is skipped and said to be. */
	void (*set_cold)(bool cold);

	/* The marker the witness watches. The beacon runs before any row and the resting
	 * pattern after the last, and the resting pattern never returns. NULL on a host, where
	 * there is no pin and no witness. */
	void (*marker_beacon)(void);
	void (*marker_rest)(void);
} measure_campaign_t;

/* Runs every operation, prints every capture and every verdict, and returns 0 when every
 * verdict behaved and 1 otherwise. When marker_rest is set and the run completed, it does not
 * return at all: the board toggles for as long as it is powered. */
int measure_campaign_run(const measure_campaign_t *c);

#endif /* MEASURE_CAMPAIGN_H */
