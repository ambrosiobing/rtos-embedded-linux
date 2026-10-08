/* projects/04-what-the-kernel-primitives-cost/zephyr/src/mutex_case.c: criterion 4.
 *
 * THE ONLY ROW IN THE TABLE WHOSE ANSWER DEPENDS ON A PROTOCOL RATHER THAN ON A COUNT, and the
 * only one with a prediction attached before the run.
 *
 * Three threads, and the arrangement is the whole measurement:
 *
 *   measurer  priority 5, highest. Wants the lock
 *   medium    priority 6. Runnable, holds nothing, and is the entire point
 *   holder    priority 7, lowest. Holds the lock and is doing a bounded amount of work
 *
 * With priority inheritance the holder is lifted to the measurer's priority while the measurer
 * waits, so it runs ahead of medium, finishes and releases. The measurer waits for the holder's
 * remaining work and nothing else.
 *
 * Without it the holder stays at priority 7, medium at priority 6 runs first and to completion,
 * and only then does the holder get to finish and release. **The measurer waits for a thread
 * that has nothing to do with the lock it wants.** That is priority inversion, and the number
 * is how much it costs.
 *
 * THE PREDICTION, WRITTEN BEFORE THE FIRST RUN. Inheritance on should cost about the holder's
 * remaining work. Inheritance off should cost about that plus the whole of medium's work.
 * Medium is given four times the holder's work, so the two figures should differ by a factor of
 * roughly four or five. **Criterion 4 is refuted if they come out the same**, and the honest
 * reading of that would be that this case is not actually contended rather than that
 * inheritance does nothing.
 *
 * WHAT IS BEING COMPARED, SAID PLAINLY BECAUSE IT IS EASY TO MISREAD. Zephyr's `k_mutex` always
 * inherits priority and offers no switch, so the second arm is not the same object configured
 * differently: it is a binary semaphore used as a lock, which is the ordinary way to hold mutual
 * exclusion without a protocol. **The comparison is between two protocols, not between two
 * settings of one object**, and the chapter has to say so or the row will be read as a claim
 * about a flag that does not exist.
 */
#include "mutex_case.h"

#include "../../adapter/measure.h"

#include <zephyr/kernel.h>

#define PRIO_MEASURER 5
#define PRIO_MEDIUM   6
#define PRIO_HOLDER   7

#define STACK_SIZE 1024

/* Work, in loop iterations rather than in microseconds, because the point is the ratio between
 * the two and a figure in microseconds would need a calibration this case does not have.
 * Medium is given four times the holder's, which is what makes the prediction checkable. */
#define HOLD_SPIN   2000u
#define MEDIUM_SPIN 8000u

K_THREAD_STACK_DEFINE(holder_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(medium_stack, STACK_SIZE);
static struct k_thread holder_thread;
static struct k_thread medium_thread;

static K_MUTEX_DEFINE(inheriting_lock);
static K_SEM_DEFINE(plain_lock, 1, 1);

static K_SEM_DEFINE(start_hold, 0, 1);
static K_SEM_DEFINE(lock_taken, 0, 1);
static K_SEM_DEFINE(start_medium, 0, 1);
static K_SEM_DEFINE(medium_done, 0, 1);

/* Which arm is running. Read by the holder, written by the measurer between arms. */
static volatile bool inheriting;
static volatile bool threads_started;

static void spin(uint32_t iterations)
{
	volatile uint32_t sink = 0u;
	uint32_t i;

	for (i = 0u; i < iterations; i++) {
		sink = sink + 1u;
	}
}

static void lock_take(void)
{
	if (inheriting) {
		(void)k_mutex_lock(&inheriting_lock, K_FOREVER);
	} else {
		(void)k_sem_take(&plain_lock, K_FOREVER);
	}
}

static void lock_release(void)
{
	if (inheriting) {
		(void)k_mutex_unlock(&inheriting_lock);
	} else {
		k_sem_give(&plain_lock);
	}
}

static void holder_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		k_sem_take(&start_hold, K_FOREVER);
		lock_take();
		/* Announce BEFORE the work, not after. The measurer has to attempt the lock
		 * while this thread is still inside the work, and announcing afterwards would
		 * leave nothing to contend for. */
		k_sem_give(&lock_taken);
		spin(HOLD_SPIN);
		lock_release();
	}
}

static void medium_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		k_sem_take(&start_medium, K_FOREVER);
		spin(MEDIUM_SPIN);
		k_sem_give(&medium_done);
	}
}

void mutex_case_run(bool with_inheritance, uint32_t *counts, size_t n)
{
	size_t i;

	if (!threads_started) {
		k_thread_create(&holder_thread, holder_stack, STACK_SIZE, holder_fn,
				NULL, NULL, NULL, PRIO_HOLDER, 0, K_NO_WAIT);
		k_thread_create(&medium_thread, medium_stack, STACK_SIZE, medium_fn,
				NULL, NULL, NULL, PRIO_MEDIUM, 0, K_NO_WAIT);
		threads_started = true;
	}

	inheriting = with_inheritance;

	for (i = 0u; i < n; i++) {
		/* The holder is lower priority, so this does not run it. */
		k_sem_give(&start_hold);

		/* Blocking here is what lets the holder run and acquire. It gives lock_taken
		 * from inside its work, and this thread preempts it immediately on the give,
		 * which leaves the holder runnable, mid-work, still holding the lock. */
		k_sem_take(&lock_taken, K_FOREVER);

		/* Medium becomes runnable now, after the lock is held and before it is wanted.
		 * It is lower priority than this thread, so it does not run yet. */
		k_sem_give(&start_medium);

		counts[i] = measure_now();
		lock_take();
		counts[i] = measure_now() - counts[i];
		lock_release();

		/* Medium has to finish before the next iteration, or its work would leak into
		 * the following measurement. In the inheriting arm it has not run at all yet. */
		k_sem_take(&medium_done, K_FOREVER);
	}
}
