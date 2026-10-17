/* projects/04-what-the-kernel-primitives-cost/freertos/mutex_case.h
 *
 * Criterion 4 under FreeRTOS, in its own translation unit for the same reason the Zephyr one
 * is: its sequencing is the measurement, and three tasks handing a lock between them is
 * easier to read on one page than interleaved with four unrelated operations.
 *
 * The prototype is identical to the Zephyr adapter's and the two headers are deliberately not
 * shared: each kernel directory knows only its own kernel, and a header under adapter/ would be
 * the first file both built from, which is where drift between two adapters begins.
 */
#ifndef MUTEX_CASE_H
#define MUTEX_CASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Creates the holder and medium tasks, which must happen before the scheduler starts. */
bool mutex_case_init(void);

/* Fills n elapsed counts for the contended case. `with_inheritance` selects a FreeRTOS mutex,
 * which inherits priority and has no switch to stop it, against a binary semaphore used as a
 * lock, which does not. Two protocols rather than two settings of one object, and the caller
 * is expected to say so. */
void mutex_case_run(bool with_inheritance, uint32_t *counts, size_t n);

#endif /* MUTEX_CASE_H */
