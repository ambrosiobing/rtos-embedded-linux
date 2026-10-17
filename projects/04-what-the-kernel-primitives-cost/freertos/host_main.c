/* projects/04-what-the-kernel-primitives-cost/freertos/host_main.c: what only FreeRTOS needs,
 * plus the verdicts a host run can honestly give.
 *
 * Three things a FreeRTOS host build has to supply and no other kernel does: the memory its own
 * idle and timer tasks need, somewhere a failed kernel assertion can be seen, and a task to
 * run the seven operations on. The operations themselves are in measure_adapter.c and
 * mutex_case.c; the capture format is in ../adapter/measure.c, which knows no kernel.
 *
 * WHAT THIS RUN CAN SAY, AND WHAT IT CANNOT. Every capture it emits carries `clock_hz 0` and the
 * reduction refuses it, so no figure here is a measurement and none goes near RESULTS.md. The
 * verdicts at the end are about behaviour under a real kernel: that every operation completed,
 * that no bracket read zero or wrapped, and that the inheriting arm of the contended case waited
 * for less than the other arm did. The last one is criterion 4's refutation with no board, and
 * it is the one that could fail if the mutex did not inherit.
 */
#include "../adapter/measure.h"
#include "mutex_case.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

#define SAMPLES 64u

bool measure_adapter_init(void);

/* A failed kernel assertion stops the run and says where. */
void p04_freertos_assert(const char *file, unsigned long line)
{
	printf("# FAIL kernel assertion at %s:%lu\n", file, line);
	fflush(stdout);
	exit(2);
}

/* configSUPPORT_DYNAMIC_ALLOCATION is 0, so FreeRTOS asks the application for the memory of
 * its own idle and timer tasks. The third parameter is configSTACK_DEPTH_TYPE, as the kernel
 * spells it. */
static StaticTask_t idle_control;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxTaskTCB, StackType_t **ppxTaskStack,
				   configSTACK_DEPTH_TYPE *puxTaskStackSize)
{
	*ppxTaskTCB = &idle_control;
	*ppxTaskStack = idle_stack;
	*puxTaskStackSize = (configSTACK_DEPTH_TYPE)configMINIMAL_STACK_SIZE;
}

static StaticTask_t timer_control;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB, StackType_t **ppxTimerStack,
				    configSTACK_DEPTH_TYPE *puxTimerStackSize)
{
	*ppxTimerTCB = &timer_control;
	*ppxTimerStack = timer_stack;
	*puxTimerStackSize = (configSTACK_DEPTH_TYPE)configTIMER_TASK_STACK_DEPTH;
}

/* ---------------------------------------------------------------------- the run */

static void measurer_fn(void *arg)
{
	static uint32_t counts[SAMPLES];
	uint32_t mins[MEASURE_OP_COUNT];
	bool completed[MEASURE_OP_COUNT];
	bool sound[MEASURE_OP_COUNT];
	int failures = 0;
	int op;

	(void)arg;

	printf("# p04 under freertos, on a host\n");
	printf("# built %s %s\n", __DATE__, __TIME__);
	printf("# THE CLOCK IS UNCONFIRMED FOR THIS BUILD. Every capture below carries\n");
	printf("# clock_hz 0 and the reduction refuses it. That is the point: these\n");
	printf("# counts are not times and nothing should be able to publish them.\n");
	printf("# configuration that bears on the brackets\n");
	printf("#   configUSE_PREEMPTION %d, configUSE_TIME_SLICING %d, configUSE_MUTEXES %d\n",
	       configUSE_PREEMPTION, configUSE_TIME_SLICING, configUSE_MUTEXES);
	printf("#   configTICK_RATE_HZ %lu, timer daemon at priority %d above the measurer at %d\n",
	       (unsigned long)configTICK_RATE_HZ, (int)configTIMER_TASK_PRIORITY,
	       (int)(tskIDLE_PRIORITY + 3));
	printf("#   instrument: the monotonic clock in hundreds of nanoseconds, rate reported 0\n");

	for (op = 0; op < (int)MEASURE_OP_COUNT; op++) {
		measure_summary_t s;
		int rc = measure_run((measure_op_t)op, counts, SAMPLES);

		printf("\n");
		completed[op] = (rc == 0);
		if (rc != 0) {
			printf("# %s: not measured in this build, %d\n",
			       measure_op_name((measure_op_t)op), rc);
			sound[op] = false;
			mins[op] = 0u;
			continue;
		}
		(void)measure_emit((measure_op_t)op, counts, SAMPLES);
		s = measure_summarise(counts, SAMPLES);
		sound[op] = s.wrap_ok && s.nonzero;
		mins[op] = s.min;
	}

	printf("\n# %d operations attempted\n", (int)MEASURE_OP_COUNT);
	printf("\n# host verdicts, about behaviour and not about figures\n");

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

	/* CRITERION 4'S REFUTATION, WITH NO BOARD. With inheritance the measurer waits for the
	 * holder's remaining work; without it, for the medium task's whole spin as well, and
	 * the medium spin is four times the holder's. A mutex that did not inherit would put
	 * both arms at the larger figure and the ratio near one. Two is the threshold: well
	 * below the four to five the work predicts, well above the one a broken protocol gives,
	 * and the host's switching cost dilutes the ratio by an amount this file cannot
	 * know. */
	if (completed[MEASURE_OP_MUTEX_INHERIT_ON] && completed[MEASURE_OP_MUTEX_INHERIT_OFF] &&
	    mins[MEASURE_OP_MUTEX_INHERIT_ON] > 0u) {
		uint32_t on = mins[MEASURE_OP_MUTEX_INHERIT_ON];
		uint32_t off = mins[MEASURE_OP_MUTEX_INHERIT_OFF];

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
	fflush(stdout);
	exit(failures == 0 ? 0 : 1);
}

static StaticTask_t measurer_control;
static StackType_t measurer_stack[configMINIMAL_STACK_SIZE * 2];

int main(void)
{
	if (!measure_adapter_init()) {
		printf("# FAIL the adapter could not create its static objects\n");
		return 1;
	}

	if (xTaskCreateStatic(measurer_fn, "measurer", configMINIMAL_STACK_SIZE * 2, NULL,
			      tskIDLE_PRIORITY + 3, measurer_stack, &measurer_control) == NULL) {
		printf("# FAIL the measurer task could not be created\n");
		return 1;
	}

	vTaskStartScheduler();

	/* Only reached if the scheduler returns, which it should not. */
	printf("# FAIL the scheduler returned\n");
	return 1;
}
