/* projects/04-what-the-kernel-primitives-cost/zephyr/src/mutex_case.h
 *
 * Criterion 4 lives in its own translation unit because its sequencing is the measurement, and
 * three threads handing a lock between them is easier to read on one page than interleaved with
 * four unrelated operations.
 */
#ifndef MUTEX_CASE_H
#define MUTEX_CASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Fills n elapsed counts for the contended case. `with_inheritance` selects a k_mutex, which
 * always inherits, against a binary semaphore used as a lock, which does not. Those are two
 * protocols rather than two settings of one object, and the caller is expected to say so. */
void mutex_case_run(bool with_inheritance, uint32_t *counts, size_t n);

#endif /* MUTEX_CASE_H */
