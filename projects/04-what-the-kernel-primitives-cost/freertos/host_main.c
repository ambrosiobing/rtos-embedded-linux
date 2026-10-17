/* projects/04-what-the-kernel-primitives-cost/freertos/host_main.c: what only a FreeRTOS host
 * build needs.
 *
 * Three things, and no measurement: the memory the kernel's own idle and timer tasks need,
 * somewhere a failed kernel assertion can be seen, and a task to run the campaign on. The
 * campaign is in ../adapter/campaign.c and is the same one the boards run; the hooks it is
 * handed here are mostly NULL, which it reports as capabilities this build lacks.
 *
 * Every capture this build emits carries `clock_hz 0` and the reduction refuses it, so no
 * figure here is a measurement and none goes near RESULTS.md. The verdicts are about behaviour
 * under a real kernel, and the one that could fail is criterion 4's refutation.
 */
#include "../adapter/campaign.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

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

static void report(void)
{
	printf("# configuration that bears on the brackets\n");
	printf("#   configUSE_PREEMPTION %d, configUSE_TIME_SLICING %d, configUSE_MUTEXES %d\n",
	       configUSE_PREEMPTION, configUSE_TIME_SLICING, configUSE_MUTEXES);
	printf("#   configTICK_RATE_HZ %lu, timer daemon at priority %d above the measurer at %d\n",
	       (unsigned long)configTICK_RATE_HZ, (int)configTIMER_TASK_PRIORITY,
	       (int)(tskIDLE_PRIORITY + 3));
	printf("#   instrument: the monotonic clock in hundreds of nanoseconds, rate reported 0\n");
}

static void measurer_fn(void *arg)
{
	static const measure_campaign_t campaign = {
		.kernel = "freertos, on a host",
		.built = __DATE__ " " __TIME__,
		.report = report,
		/* No gates: the port declares no clock. No cold pass: no cache. No marker: no
		 * pin and no witness. The campaign says each of those rather than pretending. */
		.core_clock_agrees = NULL,
		.wall_clock_agrees = NULL,
		.set_cold = NULL,
		.marker_beacon = NULL,
		.marker_rest = NULL,
	};

	(void)arg;
	fflush(stdout);
	exit(measure_campaign_run(&campaign));
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
