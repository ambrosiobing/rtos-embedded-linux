/* projects/04-what-the-kernel-primitives-cost/freertos/mutex_case.c: criterion 4 under FreeRTOS.
 *
 * THE SAME THREE TASKS AND THE SAME ARRANGEMENT AS THE ZEPHYR ADAPTER, because the arrangement
 * is the measurement:
 *
 *   measurer  priority 3, highest of the three. Wants the lock
 *   medium    priority 2. Runnable, holds nothing, and is the entire point
 *   holder    priority 1, lowest. Holds the lock and is doing a bounded amount of work
 *
 * FreeRTOS numbers priorities the other way up from Zephyr, so these are 3, 2, 1 where the
 * Zephyr file has 5, 6, 7. Same order, same three roles.
 *
 * With priority inheritance the holder is lifted to the measurer's priority while the measurer
 * waits, so it runs ahead of medium, finishes and releases. Without it the holder stays at 1,
 * medium at 2 runs first and to completion, and only then does the holder get to finish. **The
 * measurer waits for a task that has nothing to do with the lock it wants.** That is priority
 * inversion, and the number is how much it costs.
 *
 * WHAT IS BEING COMPARED, SAID PLAINLY. A FreeRTOS mutex inherits priority under
 * configUSE_MUTEXES and cannot be told not to, exactly as Zephyr's k_mutex cannot. So the
 * second arm is not the same object configured differently: it is a binary semaphore used as a
 * lock, the ordinary way to hold mutual exclusion without a protocol. **Two protocols, not two
 * settings of one object**, in both kernels, and the record has to say so.
 *
 * FreeRTOS documents its inheritance as basic: one level, released on the unlock, no ceiling.
 * That is sufficient here because there is one lock, one holder and one waiter, which is the
 * case the protocol was designed for. A case that needed transitive inheritance would be a
 * different measurement and would want saying.
 *
 * THE SPIN COUNTS ARE OVERRIDABLE, AND THE HOST BUILD OVERRIDES THEM. The defaults below are the
 * Zephyr adapter's, so a board build of this file prices the same work. The POSIX port switches
 * tasks through signals and condition variables, which costs tens of microseconds per switch,
 * and against that the Zephyr spin counts are a few microseconds of work: the inversion would
 * be lost in the switching. The host Makefile passes counts two hundred and fifty times larger, keeping
 * the ratio of four to one that makes the prediction checkable. **The host figures are not
 * comparable with the board's and are refused by the reduction in any case**; what the host
 * run shows is the direction.
 */
#include "mutex_case.h"

#include "../adapter/measure.h"
#include "port.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define PRIO_MEASURER (tskIDLE_PRIORITY + 3)
#define PRIO_MEDIUM   (tskIDLE_PRIORITY + 2)
#define PRIO_HOLDER   (tskIDLE_PRIORITY + 1)

#define STACK_WORDS configMINIMAL_STACK_SIZE

#ifndef HOLD_SPIN
#define HOLD_SPIN   2000u
#endif
#ifndef MEDIUM_SPIN
#define MEDIUM_SPIN 8000u
#endif

static StaticTask_t holder_control;
static StackType_t holder_stack[STACK_WORDS];
static StaticTask_t medium_control;
static StackType_t medium_stack[STACK_WORDS];

static StaticSemaphore_t inheriting_storage;
static SemaphoreHandle_t inheriting_lock;
static StaticSemaphore_t plain_storage;
static SemaphoreHandle_t plain_lock;

static StaticSemaphore_t start_hold_storage;
static SemaphoreHandle_t start_hold;
static StaticSemaphore_t lock_taken_storage;
static SemaphoreHandle_t lock_taken;
static StaticSemaphore_t start_medium_storage;
static SemaphoreHandle_t start_medium;
static StaticSemaphore_t medium_done_storage;
static SemaphoreHandle_t medium_done;

/* Which arm is running. Read by the holder, written by the measurer between arms. */
static volatile bool inheriting;

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
		(void)xSemaphoreTake(inheriting_lock, portMAX_DELAY);
	} else {
		(void)xSemaphoreTake(plain_lock, portMAX_DELAY);
	}
}

static void lock_release(void)
{
	if (inheriting) {
		(void)xSemaphoreGive(inheriting_lock);
	} else {
		(void)xSemaphoreGive(plain_lock);
	}
}

static void holder_fn(void *arg)
{
	(void)arg;

	for (;;) {
		(void)xSemaphoreTake(start_hold, portMAX_DELAY);
		lock_take();
		/* Announce BEFORE the work, not after. The measurer has to attempt the lock
		 * while this task is still inside the work, and announcing afterwards would
		 * leave nothing to contend for. */
		(void)xSemaphoreGive(lock_taken);
		spin(HOLD_SPIN);
		lock_release();
	}
}

static void medium_fn(void *arg)
{
	(void)arg;

	for (;;) {
		(void)xSemaphoreTake(start_medium, portMAX_DELAY);
		spin(MEDIUM_SPIN);
		(void)xSemaphoreGive(medium_done);
	}
}

bool mutex_case_init(void)
{
	inheriting_lock = xSemaphoreCreateMutexStatic(&inheriting_storage);
	plain_lock = xSemaphoreCreateBinaryStatic(&plain_storage);
	start_hold = xSemaphoreCreateBinaryStatic(&start_hold_storage);
	lock_taken = xSemaphoreCreateBinaryStatic(&lock_taken_storage);
	start_medium = xSemaphoreCreateBinaryStatic(&start_medium_storage);
	medium_done = xSemaphoreCreateBinaryStatic(&medium_done_storage);

	if (inheriting_lock == NULL || plain_lock == NULL || start_hold == NULL ||
	    lock_taken == NULL || start_medium == NULL || medium_done == NULL) {
		return false;
	}

	/* A binary semaphore is created empty and a lock starts free, so the one used as a
	 * lock is given once here. The mutex is created available. */
	(void)xSemaphoreGive(plain_lock);

	if (xTaskCreateStatic(holder_fn, "holder", STACK_WORDS, NULL, PRIO_HOLDER,
			      holder_stack, &holder_control) == NULL) {
		return false;
	}
	if (xTaskCreateStatic(medium_fn, "medium", STACK_WORDS, NULL, PRIO_MEDIUM,
			      medium_stack, &medium_control) == NULL) {
		return false;
	}
	return true;
}

void mutex_case_run(bool with_inheritance, uint32_t *counts, size_t n)
{
	size_t i;

	inheriting = with_inheritance;

	for (i = 0u; i < n; i++) {
		/* The holder is lower priority, so this does not run it. */
		(void)xSemaphoreGive(start_hold);

		/* Blocking here is what lets the holder run and acquire. It gives lock_taken
		 * from inside its work, and this task preempts it immediately on the give,
		 * which leaves the holder runnable, mid-work, still holding the lock. */
		(void)xSemaphoreTake(lock_taken, portMAX_DELAY);

		/* Medium becomes runnable now, after the lock is held and before it is wanted.
		 * It is lower priority than this task, so it does not run yet. */
		(void)xSemaphoreGive(start_medium);

		/* CRITERION 3 FOR THIS ROW, and the placement is the lesson of Friday 9
		 * October 2026 rather than a guess. The bracket opens on the next statement,
		 * in this task, so the invalidation goes immediately before it and outside
		 * it. Everything the bracket then contains starts cold: this task's path into
		 * the lock, the holder's remaining work, the switches, and in the
		 * non-inheriting arm the whole of medium's work.
		 *
		 * WHAT THAT MAKES THE COLD FIGURE, said here because the figure invites a
		 * misreading. The spins inside the bracket run thousands of iterations
		 * through a handful of instructions, so they are cold for one iteration and
		 * warm for the rest. The penalty is therefore a one-time refill of the code
		 * path and not a proportional cost, which is why it will read as a few per
		 * cent here and as more than a hundred on the short rows. */
		measure_port_chill();

		counts[i] = measure_now();
		lock_take();
		counts[i] = measure_now() - counts[i];
		lock_release();

		/* Medium has to finish before the next iteration, or its work would leak into
		 * the following measurement. In the inheriting arm it has not run at all yet. */
		(void)xSemaphoreTake(medium_done, portMAX_DELAY);
	}
}
