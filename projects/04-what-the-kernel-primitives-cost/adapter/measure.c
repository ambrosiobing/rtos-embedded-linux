/* projects/04-what-the-kernel-primitives-cost/adapter/measure.c: the core, which knows no kernel.
 *
 * NO KERNEL HEADER, NO VENDOR HEADER, NO PROCESSOR HEADER. Only stdio for the capture and the
 * contract next door. That is the whole reason this file exists separately: it compiles on a
 * host, so the capture format can be exercised against the reduction that parses it without a
 * board being involved, and a drift between the two is found by continuous integration rather
 * than on a bench evening.
 *
 * It is also the only file that applies the wrap guard, so there is one place to read rather
 * than two to keep in agreement.
 */
#include "measure.h"

#include <stdio.h>

/* Character for character the Operation column of docs/RESULTS.md. If a name changes here it
 * changes there, and the table's checker reads that column, so the two are tied by the string
 * rather than by anyone remembering. */
static const char *const NAMES[MEASURE_OP_COUNT] = {
	[MEASURE_OP_YIELD_EQUAL] = "yield to an equal-priority ready thread",
	[MEASURE_OP_BLOCK_ONE] = "block on one object, be signalled, return",
	[MEASURE_OP_BLOCK_SEVERAL] = "block on several objects, be signalled, return",
	[MEASURE_OP_HAND_TO_QUEUE] = "hand work to a queue rather than do it in place",
	[MEASURE_OP_MUTEX_INHERIT_ON] = "contended mutex, priority inheritance on",
	[MEASURE_OP_MUTEX_INHERIT_OFF] = "contended mutex, priority inheritance off",
	[MEASURE_OP_PERIOD] = "periodic thread period",
};

const char *measure_op_name(measure_op_t op)
{
	if (op < 0 || op >= MEASURE_OP_COUNT) {
		return "unknown operation";
	}
	return NAMES[op];
}

measure_summary_t measure_summarise(const uint32_t *counts, size_t n)
{
	measure_summary_t s = { .wrap_ok = true, .n = n, .min = 0u, .max = 0u };
	size_t i;

	if (counts == NULL || n == 0u) {
		/* NO SAMPLES IS NOT A PASSED GUARD. An empty run has nothing to assert about, and
		 * reporting `ok` for it would let a capture that measured nothing look sound. */
		s.wrap_ok = false;
		return s;
	}

	s.min = counts[0];
	s.max = counts[0];

	for (i = 0u; i < n; i++) {
		if (counts[i] < s.min) {
			s.min = counts[i];
		}
		if (counts[i] > s.max) {
			s.max = counts[i];
		}
		/* A zero elapsed count is not a fast operation. It is a region that was never
		 * bracketed, or one bracketed around nothing, and either way it is not a
		 * measurement. */
		if (counts[i] == 0u || counts[i] >= MEASURE_WRAP_LIMIT) {
			s.wrap_ok = false;
		}
	}

	return s;
}

size_t measure_emit(measure_op_t op, const uint32_t *counts, size_t n)
{
	measure_summary_t s = measure_summarise(counts, n);
	size_t i;

	printf("# p04 device half, %s\n", measure_op_name(op));
	printf("# the host adds resolution_s and b_edges_s from the witness before reducing\n");

	/* The clock goes in whatever it is, INCLUDING ZERO. An unconfirmed clock tree reports
	 * zero here and the reduction refuses the capture, which is the outcome we want: the
	 * refusal travels with the data rather than depending on whoever reads it. */
	printf("clock_hz %u\n", (unsigned int)measure_clock_hz());
	printf("wrap_guard %s\n", s.wrap_ok ? "ok" : "unknown");

	printf("a_counts");
	for (i = 0u; i < n; i++) {
		printf(" %u", (unsigned int)counts[i]);
	}
	printf("\n");

	return n;
}
