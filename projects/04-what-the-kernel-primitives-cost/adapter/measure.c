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

/* WHICH COUNTER EACH BRACKET NEEDS, decided by whether the bracket contains a sleep. Six do
 * not and one does. The table is here, beside the names, because it is a fact about the
 * operation and not about any kernel: see the contract for why. */
static const measure_instrument_t INSTRUMENTS[MEASURE_OP_COUNT] = {
	[MEASURE_OP_YIELD_EQUAL] = MEASURE_INSTRUMENT_CORE,
	[MEASURE_OP_BLOCK_ONE] = MEASURE_INSTRUMENT_CORE,
	[MEASURE_OP_BLOCK_SEVERAL] = MEASURE_INSTRUMENT_CORE,
	[MEASURE_OP_HAND_TO_QUEUE] = MEASURE_INSTRUMENT_CORE,
	[MEASURE_OP_MUTEX_INHERIT_ON] = MEASURE_INSTRUMENT_CORE,
	[MEASURE_OP_MUTEX_INHERIT_OFF] = MEASURE_INSTRUMENT_CORE,
	[MEASURE_OP_PERIOD] = MEASURE_INSTRUMENT_WALL,
};

static const char *const INSTRUMENT_NAMES[MEASURE_INSTRUMENT_COUNT] = {
	[MEASURE_INSTRUMENT_CORE] = "core",
	[MEASURE_INSTRUMENT_WALL] = "wall",
};

measure_instrument_t measure_op_instrument(measure_op_t op)
{
	if (op < 0 || op >= MEASURE_OP_COUNT) {
		return MEASURE_INSTRUMENT_CORE;
	}
	return INSTRUMENTS[op];
}

const char *measure_instrument_name(measure_instrument_t instrument)
{
	if (instrument < 0 || instrument >= MEASURE_INSTRUMENT_COUNT) {
		return "unknown";
	}
	return INSTRUMENT_NAMES[instrument];
}

measure_summary_t measure_summarise(const uint32_t *counts, size_t n)
{
	measure_summary_t s = { .wrap_ok = true, .nonzero = true, .n = n, .min = 0u, .max = 0u };
	size_t i;

	if (counts == NULL || n == 0u) {
		/* NO SAMPLES IS NOT A PASSED GUARD. An empty run has nothing to assert about, and
		 * reporting `ok` for it would let a capture that measured nothing look sound. */
		s.wrap_ok = false;
		s.nonzero = false;
		s.n = 0u;
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
		 * measurement. It is recorded SEPARATELY from the wrap, because the two have
		 * different causes and a capture that names the wrong one sends its reader to
		 * the wrong place. */
		if (counts[i] == 0u) {
			s.nonzero = false;
		}
		if (counts[i] >= MEASURE_WRAP_LIMIT) {
			s.wrap_ok = false;
		}
	}

	return s;
}

const char *measure_guard_name(measure_summary_t summary)
{
	/* Order matters only in that a capture reports one fault, and the one it reports should
	 * be the one a reader can act on. No samples is checked first because with none, neither
	 * of the other two flags means anything. */
	if (summary.n == 0u) {
		return "unknown";
	}
	if (!summary.nonzero) {
		return "zero";
	}
	if (!summary.wrap_ok) {
		return "wrapped";
	}
	return "ok";
}

size_t measure_emit(measure_op_t op, const uint32_t *counts, size_t n)
{
	measure_summary_t s = measure_summarise(counts, n);
	size_t i;

	printf("# p04 device half, %s\n", measure_op_name(op));
	printf("# the host adds resolution_s and b_edges_s from the witness before reducing\n");

	/* THE INSTRUMENT AND ITS RATE TRAVEL TOGETHER, from one lookup, so that a row measured
	 * with one counter cannot be labelled with the other's rate. The clock goes in whatever
	 * it is, INCLUDING ZERO: an unconfirmed clock tree reports zero here and the reduction
	 * refuses the capture, which is the outcome we want, because the refusal travels with the
	 * data rather than depending on whoever reads it. */
	{
		measure_instrument_t inst = measure_op_instrument(op);
		uint32_t hz = (inst == MEASURE_INSTRUMENT_WALL) ? measure_clock_hz_wall()
							       : measure_clock_hz();

		printf("instrument %s\n", measure_instrument_name(inst));
		printf("clock_hz %u\n", (unsigned int)hz);
	}
	printf("wrap_guard %s\n", measure_guard_name(s));

	printf("a_counts");
	for (i = 0u; i < n; i++) {
		printf(" %u", (unsigned int)counts[i]);
	}
	printf("\n");

	return n;
}
