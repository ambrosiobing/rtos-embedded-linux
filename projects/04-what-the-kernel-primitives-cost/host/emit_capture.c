/* projects/04-what-the-kernel-primitives-cost/host/emit_capture.c: the core, on a host.
 *
 * WHY THIS EXISTS. The firmware emits a capture and reduce.py parses one. Those are two
 * descriptions of the same format in two languages, which is two chances to write a different
 * format, and no test inside either of them can notice the drift because each is right about
 * itself. The volume has met that shape before, in P01's table and P02's cascade.
 *
 * So the core is compiled here with a host port, made to emit captures, and the output is fed
 * to the reduction. If the emitter and the parser ever disagree, continuous integration says
 * so, without a board and without an evening.
 *
 * It provides the PORT half of the contract and not the adapter half. measure_run() belongs to
 * a kernel and there is no kernel here; a stub that returned plausible counts would be a second
 * implementation of the thing under test, which is the mistake this file exists to catch.
 *
 *   usage: emit_capture <op index> <mode>
 *   modes: clean, wrapped, zeroclock, zerocount
 *
 * Three of those four modes are refusals, and they are here so that the refusal is shown to
 * travel in the capture rather than depending on whoever reads it.
 */
#include "../adapter/measure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLES    40u
#define NOMINAL    280000u      /* one millisecond at 280 MHz */
#define HOST_CLOCK 280000000u

static uint32_t clock_hz = HOST_CLOCK;

uint32_t measure_now(void)
{
	/* The host has no cycle counter worth the name. Nothing in this file brackets a real
	 * region, so this advances rather than pretending to measure. */
	static uint32_t t;

	t += NOMINAL;
	return t;
}

uint32_t measure_clock_hz(void)
{
	return clock_hz;
}

/* The wall instrument, on a host where neither counter is real. Same advancing stub and the
 * same rate, so that the only thing the pipeline test can be checking is the format: whether
 * the emitter labels a period capture `wall` and the parser insists on it. */
uint32_t measure_now_wall(void)
{
	return measure_now();
}

uint32_t measure_clock_hz_wall(void)
{
	return clock_hz;
}

int main(int argc, char **argv)
{
	uint32_t counts[SAMPLES];
	unsigned int i;
	long op;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <op index> <clean|wrapped|zeroclock|zerocount>\n", argv[0]);
		return 2;
	}

	op = strtol(argv[1], NULL, 10);
	if (op < 0 || op >= (long)MEASURE_OP_COUNT) {
		fprintf(stderr, "op index %ld is outside 0 to %d\n", op, (int)MEASURE_OP_COUNT - 1);
		return 2;
	}

	for (i = 0u; i < SAMPLES; i++) {
		counts[i] = NOMINAL + (i % 7u) * 200u;
	}

	if (strcmp(argv[2], "clean") == 0) {
		/* nothing to spoil */
	} else if (strcmp(argv[2], "wrapped") == 0) {
		counts[SAMPLES / 2u] = MEASURE_WRAP_LIMIT + 1u;
	} else if (strcmp(argv[2], "zerocount") == 0) {
		counts[SAMPLES - 1u] = 0u;
	} else if (strcmp(argv[2], "zeroclock") == 0) {
		clock_hz = 0u;
	} else {
		fprintf(stderr, "unknown mode %s\n", argv[2]);
		return 2;
	}

	(void)measure_emit((measure_op_t)op, counts, SAMPLES);
	return 0;
}
