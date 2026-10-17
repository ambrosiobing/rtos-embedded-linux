/* projects/04-what-the-kernel-primitives-cost/adapter/campaign.c: the run, which knows no kernel.
 *
 * Lifted from the Zephyr main on Saturday 17 October 2026, line for line where the lines were
 * kernel-free, and every comment that explained a decision came with them. Where a comment
 * below names a date, that is the date the decision was made in the file this came from.
 *
 * NO KERNEL HEADER, NO VENDOR HEADER, NO PROCESSOR HEADER. The hooks in campaign.h are the
 * only way this file reaches a kernel, and a hook left NULL is reported as a capability the
 * build lacks rather than silently worked around.
 */
#include "campaign.h"
#include "measure.h"

#include <stdio.h>

/* CRITERION 1: THE INSTRUMENT HAS TO BE CHEAPER THAN EVERYTHING IT MEASURES.
 *
 * Two reads of the counter sit inside every bracket, and the criterion asks for at least an
 * order of magnitude between the instrument and the smallest thing it reports, because below
 * that the table is partly measuring the measurement.
 *
 * This is an empty bracket: two reads and nothing between them. It is reported beside the
 * clock check rather than as a row of the results table, because it is a property of the
 * instrument and not of the kernel, and a table row would invite somebody to compare it with
 * a primitive as though they were the same kind of thing.
 *
 * The MINIMUM is the figure that matters here, not the median. An empty bracket has a floor
 * and no ceiling: anything above the floor is interference, and the floor is what the
 * instrument actually costs.
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

/* THE SIX OPERATIONS CRITERION 3 REPEATS COLD, six since Saturday 17 October 2026 and four
 * before it. The period row is the only one left out, and for the reason that excluded three:
 * it brackets a sleep of a millisecond, so a cold measurement of it would be a cold first
 * microsecond and a warm rest, which is not the comparison the criterion asks for.
 *
 * THE SAME ARGUMENT WAS APPLIED TO THE TWO CONTENDED MUTEX ROWS AND IS WITHDRAWN. It was
 * written before either had been measured and it reads, in the version this comment replaces,
 * that those rows "take long enough that a cache state would be lost inside them". They take
 * 29.9 and 130.8 microseconds under Zephyr and 57.4 and 249.6 under FreeRTOS, so the argument
 * is not wrong about the dilution. It is wrong about what to do: the dilution is itself the
 * figure, and refusing to measure it leaves four cells of the table empty and the claim
 * untested. A prediction nobody can check is the thing this project exists not to publish.
 *
 * THE PREDICTION, WRITTEN BEFORE THE RUN, as every prediction in this project is. The spins
 * inside those brackets run thousands of iterations through a few instructions, so they go cold
 * once and warm thereafter. The penalty should therefore be a ONE-TIME REFILL OF THE CODE
 * PATH, of the same order as the short rows' penalties, and it should be ABOUT THE SAME IN
 * BOTH ARMS, because what separates the arms is spin iterations and not code. On the figures
 * already in hand that means a few hundred counts under Zephyr and one to two thousand under
 * FreeRTOS, reading as roughly 4 to 11 per cent on the inheriting arm and 1 to 3 per cent on
 * the other.
 *
 * WHAT WOULD REFUTE IT. A penalty on the non-inheriting arm much larger than on the
 * inheriting one cannot be a one-time refill, since the extra span is spin; it would mean the
 * cache state is being lost and re-lost inside the bracket, and the comment this one replaces
 * would have been right for a reason it did not state. A penalty near zero on both would mean
 * the invalidation is not reaching the path, which is how the block-on-one row was found to be
 * rewarming itself on Friday 9 October 2026. Either outcome is reportable and neither is a
 * figure to quietly drop. */
#define COLD_OPS 6

static void cold_pass(const measure_campaign_t *c, uint32_t *counts,
		      const uint32_t *warm_min)
{
	uint32_t cold_min_of[COLD_OPS];
	int op;

	c->set_cold(true);
	printf("\n# criterion 3: the same six operations with the instruction cache\n");
	printf("#   invalidated immediately before each bracket opens, outside it\n");

	/* THE DISTRIBUTION, NOT ONLY THE FLOOR, and the reason is a question the first version
	 * of this pass could not answer. On Friday 9 October 2026 one row came out four counts
	 * lower cold than warm, and whether 1468 was the floor of a tight distribution or one
	 * low sample in a scattered one could not be told, because only the minimum was printed.
	 *
	 * Printed as a comment rather than through measure_emit(). The capture format has no
	 * field for the cache state, so a second capture per operation would be
	 * indistinguishable from the first except by its position in the log, and a number whose
	 * conditions cannot be read off the record should not be in the record.
	 *
	 * The runs all happen before the table is printed, so that the table arrives in one
	 * piece with the count dumps above it rather than interleaved through it. */
	for (op = 0; op < COLD_OPS; op++) {
		size_t k;

		cold_min_of[op] = 0xFFFFFFFFu;
		if (measure_run((measure_op_t)op, counts, MEASURE_SAMPLES) != 0) {
			continue;
		}
		for (k = 0u; k < MEASURE_SAMPLES; k++) {
			if (counts[k] < cold_min_of[op]) {
				cold_min_of[op] = counts[k];
			}
		}

		printf("#\n#   cold counts, %s\n#  ", measure_op_name((measure_op_t)op));
		for (k = 0u; k < MEASURE_SAMPLES; k++) {
			printf(" %u", (unsigned int)counts[k]);
		}
		printf("\n");
	}
	c->set_cold(false);

	printf("#\n#   minimum of 64, because an operation has a floor and no ceiling\n");
	printf("#   the penalty in counts is the figure to quote, not the percentage:\n");
	printf("#   a percentage carries whatever the baseline happens to be\n");
	printf("#\n");
	printf("#   operation                                        warm    cold  penalty\n");

	for (op = 0; op < COLD_OPS; op++) {
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
			 * invalidation was in the measurer and that row's bracket opens on a
			 * stamp taken in the giver, after a give and a block had already
			 * rewarmed the path. The flag found a defect in the case. */
			printf("#   %-46s %6u  %6u        0  COLD IS LOWER, which should not happen\n",
			       measure_op_name((measure_op_t)op),
			       (unsigned int)warm_min[op], (unsigned int)cold_min_of[op]);
		}
	}
}

int measure_campaign_run(const measure_campaign_t *c)
{
	static uint32_t counts[MEASURE_SAMPLES];
	uint32_t warm_min[MEASURE_OP_COUNT];
	bool completed[MEASURE_OP_COUNT];
	bool sound[MEASURE_OP_COUNT];
	uint32_t overhead;
	uint32_t smallest = 0xFFFFFFFFu;
	bool clock_confirmed;
	int failures = 0;
	int op;

	printf("# p04 under %s\n", c->kernel);
	/* THE BUILD STAMP, AND IT EXISTS BECAUSE THE NAME ABOVE WAS NOT ENOUGH. On Friday 9
	 * October 2026 the name read correctly and the image was the previous commit's; a
	 * rebuild had been skipped, and the figures that came back were an exact repeat that
	 * looked like a reproduction. A reproduction and a stale flash produce the same numbers,
	 * and only this line separates them. It is evaluated in the kernel's own main, which is
	 * the file that changes, so a cached object for this one cannot freeze it. */
	printf("# built %s, and a stamp older than your last edit means a stale flash\n",
	       c->built);

	clock_confirmed = measure_clock_hz() != 0u && measure_clock_hz_wall() != 0u;
	if (!clock_confirmed) {
		printf("# THE CLOCK IS UNCONFIRMED FOR THIS BUILD. Every capture below carries\n");
		printf("# clock_hz 0 and the reduction refuses it. That is the point: these\n");
		printf("# counts are not times and nothing should be able to publish them.\n");
	} else if ((c->core_clock_agrees != NULL && !c->core_clock_agrees()) ||
		   (c->wall_clock_agrees != NULL && !c->wall_clock_agrees())) {
		printf("#\n# REFUSED. The counter does not advance at the rate the kernel\n");
		printf("# declares, so every count below would be scaled by an unknown\n");
		printf("# factor while looking entirely reasonable. Nothing is emitted.\n");
		return 1;
	}

	if (c->report != NULL) {
		c->report();
	}
	if (c->marker_beacon != NULL) {
		c->marker_beacon();
	}

	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		warm_min[op] = 0xFFFFFFFFu;
	}

	overhead = instrument_cost();

	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		measure_summary_t s;
		int rc = measure_run((measure_op_t)op, counts, MEASURE_SAMPLES);
		size_t k;

		printf("\n");
		completed[op] = (rc == 0);
		if (rc != 0) {
			printf("# %s: not measured in this build, %d\n",
			       measure_op_name((measure_op_t)op), rc);
			sound[op] = false;
			continue;
		}
		(void)measure_emit((measure_op_t)op, counts, MEASURE_SAMPLES);
		s = measure_summarise(counts, MEASURE_SAMPLES);
		sound[op] = s.wrap_ok && s.nonzero;

		/* One pass, two minima. Criterion 1 prices the CORE instrument against the
		 * smallest thing the core instrument measured, so a wall-instrument row does
		 * not enter that minimum: it is a different counter, and a millisecond
		 * besides. */
		for (k = 0u; k < MEASURE_SAMPLES; k++) {
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

	if (c->set_cold != NULL) {
		cold_pass(c, counts, warm_min);
	} else {
		printf("\n# criterion 3: no cold pass, this build has no instruction cache to\n");
		printf("#   invalidate. A host has no criterion 3.\n");
	}

	if (clock_confirmed && overhead > 0u && smallest != 0xFFFFFFFFu) {
		printf("# criterion 1: the instrument costs %u counts and the smallest\n",
		       (unsigned int)overhead);
		printf("#   thing it measured was %u, a ratio of %u. The criterion asks\n",
		       (unsigned int)smallest, (unsigned int)(smallest / overhead));
		printf("#   for at least ten, so this run %s\n",
		       (smallest / overhead) >= 10u ? "meets it" : "does not");
	}

	/* THE VERDICTS, ABOUT BEHAVIOUR AND NOT ABOUT FIGURES, and they run on every build
	 * including the ones whose captures are refused. Written first for the FreeRTOS host
	 * build on Saturday 17 October 2026, and then it was obvious the board had never checked
	 * these either: that every row completed, that no bracket read zero or wrapped, and the
	 * one with a built-in refutation. */
	printf("\n# verdicts, about behaviour and not about figures\n");
	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		const char *name = measure_op_name((measure_op_t)op);

		if (!completed[op]) {
			printf("#   FAIL %s did not complete\n", name);
			failures++;
		} else if (!sound[op]) {
			printf("#   FAIL %s returned a zero or wrapped count\n", name);
			failures++;
		} else {
			printf("#   ok   %s completed, every count nonzero and unwrapped\n", name);
		}
	}

	/* CRITERION 4'S REFUTATION. With inheritance the measurer waits for the holder's
	 * remaining work; without it, for the medium task's whole spin as well, and the medium
	 * spin is four times the holder's. A mutex that did not inherit would put both arms at
	 * the larger figure and the ratio near one. Two is the threshold: well below the four to
	 * five the work predicts, well above the one a broken protocol gives, and whatever
	 * switching cost sits in both arms dilutes the ratio by an amount this file cannot
	 * know. */
	if (completed[MEASURE_OP_MUTEX_INHERIT_ON] && completed[MEASURE_OP_MUTEX_INHERIT_OFF] &&
	    warm_min[MEASURE_OP_MUTEX_INHERIT_ON] > 0u) {
		uint32_t on = warm_min[MEASURE_OP_MUTEX_INHERIT_ON];
		uint32_t off = warm_min[MEASURE_OP_MUTEX_INHERIT_OFF];

		if (off > 2u * on) {
			printf("#   ok   inheritance off waited %u counts against %u with it on,"
			       " a ratio of %u.%u, so the mutex inherits\n",
			       (unsigned int)off, (unsigned int)on, (unsigned int)(off / on),
			       (unsigned int)((off % on) * 10u / on));
		} else {
			printf("#   FAIL inheritance off waited %u counts against %u with it on,"
			       " which is not the inversion the protocol predicts\n",
			       (unsigned int)off, (unsigned int)on);
			failures++;
		}
	}
	printf("# %s\n", failures == 0 ? "every verdict behaved" : "at least one verdict failed");

	/* Everything above is printed before this is reached, so a console capture is complete
	 * whether or not anybody is watching the pin. This does not return. */
	if (c->marker_rest != NULL) {
		c->marker_rest();
	}
	return failures == 0 ? 0 : 1;
}
