/* projects/04-what-the-kernel-primitives-cost/adapter/measure.h: the contract.
 *
 * THREE PARTIES, AND NONE OF THEM INCLUDES ANOTHER'S HEADER.
 *
 *   the port     provides a free-running cycle count and the rate it advances at
 *   the adapter  provides one function that performs an operation n times and fills in the
 *                elapsed counts, once for Zephyr and once for FreeRTOS
 *   the core     summarises, applies the wrap guard, and emits the capture
 *
 * The core is in measure.c and contains no kernel header, no vendor header and no processor
 * header. That is what lets it be compiled and exercised on a host, which in turn is what lets
 * the capture format be tested against the reduction that reads it, in continuous integration,
 * with no board involved. A format that is only exercised on hardware drifts from its parser
 * and nobody finds out until the bench evening.
 *
 * WHY THE OPERATIONS ARE NAMED BY WHAT HAPPENS. A queue in one kernel is not automatically the
 * queue in the other, and a timing number carries no sign of being about the wrong operation.
 * So each name below describes the event rather than the call, and the strings returned by
 * measure_op_name() are character for character the Operation column of docs/RESULTS.md, so
 * that the firmware and the table cannot drift apart silently.
 */
#ifndef MEASURE_H
#define MEASURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
	MEASURE_OP_YIELD_EQUAL = 0,
	MEASURE_OP_BLOCK_ONE,
	MEASURE_OP_BLOCK_SEVERAL,
	MEASURE_OP_HAND_TO_QUEUE,
	MEASURE_OP_MUTEX_INHERIT_ON,
	MEASURE_OP_MUTEX_INHERIT_OFF,
	MEASURE_OP_PERIOD,
	MEASURE_OP_COUNT
} measure_op_t;

/* Character for character the Operation column of docs/RESULTS.md. */
const char *measure_op_name(measure_op_t op);

/* TWO INSTRUMENTS, AND WHICH ONE A ROW USES IS A PROPERTY OF ITS BRACKET.
 *
 * Established Friday 9 October 2026. The counter that prices a kernel primitive on this part,
 * and that Zephyr's own benchmark suite uses, counts core cycles and stops when the core stops.
 * Six of the seven brackets contain no idle at all, so that is the right instrument for them.
 * The seventh brackets a sleep, and across a sleep that counter reads the microseconds the
 * wake-up took rather than the millisecond that passed. A counter that stops during a sleep
 * cannot measure a sleep.
 *
 *   core   stops when the core idles. Fine-grained, and what upstream prices with.
 *   wall   keeps running through idle. Coarser, driven by the system timer, and the only one
 *          that can bracket a sleep.
 *
 * The choice lives HERE, in the core beside the operation names, rather than in each adapter,
 * because a bracket containing a sleep is a fact about the operation and not about the kernel.
 * An adapter that picked differently would be measuring a different thing under the same name,
 * which is the drift the whole contract exists to prevent. */
typedef enum {
	MEASURE_INSTRUMENT_CORE = 0,
	MEASURE_INSTRUMENT_WALL,
	MEASURE_INSTRUMENT_COUNT
} measure_instrument_t;

measure_instrument_t measure_op_instrument(measure_op_t op);

/* "core" or "wall", character for character the `instrument` line of a capture. */
const char *measure_instrument_name(measure_instrument_t instrument);

/* Provided by the port. Free-running counts that wrap, and the rate each advances at.
 *
 * measure_now() is the CORE instrument, which every bracket without a sleep uses; the name is
 * unqualified because that is what it has always meant in the adapters. measure_now_wall() is
 * the one that survives idle, for the one bracket that needs it. */
uint32_t measure_now(void);
uint32_t measure_now_wall(void);

/* THE RATE, OR ZERO. Zero means the clock tree has not been confirmed, and the core then emits
 * `clock_hz 0`, which the reduction refuses. A count is not a time, and a guessed rate scales
 * every figure by an unknown factor while still looking like a measurement, so the refusal
 * travels in the capture rather than being left to whoever reads it.
 *
 * ONE RATE PER INSTRUMENT. The two counters on this part happen to run at the same 280 MHz,
 * and a capture states its own rate anyway, because "they happen to" is not a contract. */
uint32_t measure_clock_hz(void);
uint32_t measure_clock_hz_wall(void);

/* Provided by the kernel adapter. Fills n elapsed counts for one operation, returning 0 on
 * success and a negative value if the operation is not available under that kernel. */
int measure_run(measure_op_t op, uint32_t *counts, size_t n);

/* THE WRAP GUARD. At this processor's clock the counter wraps in about fifteen seconds, and a
 * bracketed region longer than that yields a small, stable, in-range number, which is the worst
 * kind of wrong because nothing about it looks like an error.
 *
 * Half the counter range is the limit rather than the whole of it: a region that took longer
 * than half a wrap is already far outside anything this table measures, so treating it as
 * suspect costs nothing and catches the case where a region wrapped once and came back looking
 * reasonable. */
#define MEASURE_WRAP_LIMIT 0x80000000u

typedef struct {
	bool wrap_ok;   /* every count below the limit */
	size_t n;
	uint32_t min;
	uint32_t max;
} measure_summary_t;

measure_summary_t measure_summarise(const uint32_t *counts, size_t n);

/* Emit the DEVICE HALF of a capture: instrument, clock_hz, wrap_guard and a_counts.
 *
 * The `instrument` line and the `clock_hz` beside it come from measure_op_instrument(), so a
 * row measured with one counter cannot be labelled with the other's rate. That pairing is the
 * same discipline docs/RESULTS.md applies to its Instrument column, one level down.
 *
 * It is deliberately half. The device cannot know what the external witness resolves or when it
 * saw an edge, so `resolution_s` and `b_edges_s` are added on the host from the acquisition
 * board's own capture. A half fed to the reduction by mistake is refused rather than reduced,
 * because the reduction requires all five fields, and that is the behaviour we want from an
 * accident.
 *
 * Returns the number of counts emitted. */
size_t measure_emit(measure_op_t op, const uint32_t *counts, size_t n);

#endif /* MEASURE_H */
