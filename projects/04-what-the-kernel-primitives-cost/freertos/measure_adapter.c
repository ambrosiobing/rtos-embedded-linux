/* projects/04-what-the-kernel-primitives-cost/freertos/measure_adapter.c: the FreeRTOS adapter.
 *
 * This file, mutex_case.c and the two ports are the only ones in the project that know FreeRTOS
 * exists. The core next door, ../adapter/, knows no kernel, and the Zephyr adapter in
 * ../zephyr/src/ knows only Zephyr. Each row below names the operation by what happens, and the
 * map from that to this kernel's objects is in docs/DESIGN.md, written before this file.
 *
 * THE PORT IS NOT HERE. The two counters and their rates are the contract in measure.h, and the
 * cache and the marker are the four hooks in port.h; host_port.c and board_port.c each supply
 * all of them, and this file cannot tell which it was linked with. That is what lets one adapter
 * prove its sequencing on a host with every capture refused, and then price the same brackets
 * on the board with the same code.
 *
 * EVERYTHING IS STATIC. configSUPPORT_DYNAMIC_ALLOCATION is 0, so an allocating create does
 * not fail at run time, it fails to link, and the Makefile checks the binary afterwards.
 */
#include "../adapter/campaign.h"
#include "../adapter/measure.h"
#include "mutex_case.h"
#include "port.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include <errno.h>
#include <stdio.h>

/* Higher numbers are higher priority. The measurer sits above its partner's helpers so that a
 * give returns before the other side runs, which is what makes the bracket contain the wake-up
 * rather than the hand-off. The yield partner is the measurer's equal. */
#define PRIO_MEASURER (tskIDLE_PRIORITY + 3)
#define PRIO_PARTNER  (tskIDLE_PRIORITY + 3)
#define PRIO_GIVER    (tskIDLE_PRIORITY + 1)

#define STACK_WORDS configMINIMAL_STACK_SIZE

/* ---- static objects ---------------------------------------------------------------------- */

static StaticTask_t partner_control;
static StackType_t partner_stack[STACK_WORDS];
static TaskHandle_t partner_task;

static StaticTask_t giver_control;
static StackType_t giver_stack[STACK_WORDS];

static StaticSemaphore_t go_storage;        /* the measurer releases the giver */
static SemaphoreHandle_t go;
static StaticSemaphore_t signalled_storage; /* what the measurer blocks on, one object */
static SemaphoreHandle_t signalled;
static StaticSemaphore_t work_done_storage;
static SemaphoreHandle_t work_done;

static StaticEventGroup_t events_storage;   /* what the measurer blocks on, several objects */
static EventGroupHandle_t events;
#define EVENT_SIGNALLED (1u << 0)
#define EVENT_SECOND    (1u << 1)

/* Which object the giver signals. Written by the measurer before releasing the giver. */
typedef enum { GIVE_SEMAPHORE, GIVE_EVENT } give_mode_t;
static volatile give_mode_t give_mode = GIVE_SEMAPHORE;

static volatile uint32_t t_handoff; /* written by the other side, read by the measurer */

/* ---- the partner and giver tasks --------------------------------------------------------- */

static void partner_fn(void *arg)
{
	(void)arg;

	for (;;) {
		taskYIELD();
	}
}

static void giver_fn(void *arg)
{
	(void)arg;

	for (;;) {
		(void)xSemaphoreTake(go, portMAX_DELAY);
		/* CRITERION 3 FOR BOTH BLOCKING ROWS HAPPENS HERE, not in the measurer. The
		 * bracket opens on the stamp below, so this is the last point at which an
		 * invalidation still lands outside it and ahead of it. The Zephyr adapter
		 * learned this on Friday 9 October 2026 at the cost of a run: a call at the top
		 * of the measurer's loop was followed by a give, a blocking take and this
		 * task's own wake-up, all untimed, which refilled the cache before the bracket
		 * opened. In the warm pass this is one test of a flag. */
		measure_port_chill();
		/* The last thing before the give, so the bracket holds the wake-up and not the
		 * giver's own run-up. */
		t_handoff = measure_now();
		if (give_mode == GIVE_SEMAPHORE) {
			(void)xSemaphoreGive(signalled);
		} else {
			(void)xEventGroupSetBits(events, EVENT_SIGNALLED);
		}
	}
}

/* Runs on the timer daemon task, which is FreeRTOS's counterpart to Zephyr's system work
 * queue for this row. Signature as timers.h spells it. */
static void work_fn(void *arg1, uint32_t arg2)
{
	(void)arg1;
	(void)arg2;
	t_handoff = measure_now();
	(void)xSemaphoreGive(work_done);
}

/* ---- the operations ---------------------------------------------------------------------- */

/* A FULL ROUND TRIP, not one switch. The measurer yields, the equal-priority partner runs and
 * yields back, and the bracket closes. Same bracket as the Zephyr adapter, and the same
 * precaution: the partner runs only here. An always-ready task at the measurer's priority is
 * exactly what this row needs and exactly what every other row cannot survive, because the
 * lower-priority giver would never be scheduled and the next row would block forever. */
static void run_yield(uint32_t *counts, size_t n)
{
	size_t i;

	vTaskResume(partner_task);

	for (i = 0u; i < n; i++) {
		measure_port_chill();
		uint32_t t0 = measure_now();

		taskYIELD();
		counts[i] = measure_now() - t0;
	}

	vTaskSuspend(partner_task);
}

/* FROM THE GIVE TO THE RESUMPTION. The giver is lower priority, so releasing it does not run
 * it; the measurer then blocks, the giver runs, stamps and gives, and the measurer preempts it
 * immediately. The bracket therefore contains the wake-up and not the hand-off. No chill here:
 * the giver does it, on its last statement before the stamp this bracket opens on. */
static void run_block_one(uint32_t *counts, size_t n)
{
	size_t i;

	give_mode = GIVE_SEMAPHORE;
	for (i = 0u; i < n; i++) {
		(void)xSemaphoreGive(go);
		(void)xSemaphoreTake(signalled, portMAX_DELAY);
		counts[i] = measure_now() - t_handoff;
	}
}

/* THE SAME EVENT, WAITED FOR ON A DIFFERENT OBJECT. An event group waited on for any of two
 * bits, where Zephyr polls two semaphores. The design records that this row prices the
 * alternative to one object rather than the same object waited on differently, in both
 * kernels. Clearing on exit resets the bit for the next iteration, which is the work Zephyr's
 * adapter does by hand on its poll events. */
static void run_block_several(uint32_t *counts, size_t n)
{
	size_t i;

	give_mode = GIVE_EVENT;
	for (i = 0u; i < n; i++) {
		(void)xSemaphoreGive(go);
		(void)xEventGroupWaitBits(events, EVENT_SIGNALLED | EVENT_SECOND, pdTRUE, pdFALSE,
					  portMAX_DELAY);
		counts[i] = measure_now() - t_handoff;
	}
	give_mode = GIVE_SEMAPHORE;
}

/* FROM THE PEND TO THE HANDLER RUNNING. xTimerPendFunctionCall posts to the daemon task's
 * queue; the daemon sits above the measurer, so the post preempts straight into work_fn and
 * the bracket closes on the stamp it takes there. One switch, as the Zephyr row measures, and
 * not the cost of the call, which is the figure that usually gets quoted. */
static void run_hand_to_queue(uint32_t *counts, size_t n)
{
	size_t i;

	for (i = 0u; i < n; i++) {
		measure_port_chill();
		uint32_t t0 = measure_now();

		/* A zero wait: the daemon queue holds ten and never more than one is in flight,
		 * so a refusal here is a fault to stop on rather than to retry. */
		configASSERT(xTimerPendFunctionCall(work_fn, NULL, 0u, 0) == pdPASS);
		(void)xSemaphoreTake(work_done, portMAX_DELAY);
		counts[i] = t_handoff - t0;
	}
}

/* THE PERIOD, on the wall instrument because its bracket contains a sleep, and with the marker
 * toggled beside each stamp so the witness's intervals line up one to one with a_counts. The
 * tick decides the figure, and a 1 ms request on a 1 kHz tick is one tick, which is 1.000 ms
 * against Zephyr's 1.1 on its 100 us tick. Not comparable across kernels and never meant to
 * be; the row exists for criterion 5, two instruments agreeing on one kernel's period. */
static int run_period(uint32_t *counts, size_t n)
{
	uint32_t previous;
	size_t i;

	/* NO MARKER, NO ROW. What this row exists for is the agreement of two instruments, and
	 * a capture the witness could not have seen would be reduced against nothing. */
	if (!measure_port_marker_ready()) {
		printf("# the marker pin could not be configured, so the witness has\n");
		printf("# nothing to watch and the period row is withheld\n");
		return -ENODEV;
	}

	vTaskDelay(pdMS_TO_TICKS(1));
	previous = measure_now_wall();
	measure_port_marker_toggle();

	for (i = 0u; i < n; i++) {
		vTaskDelay(pdMS_TO_TICKS(1));
		uint32_t now = measure_now_wall();

		measure_port_marker_toggle();
		counts[i] = now - previous;
		previous = now;
	}
	return 0;
}

int measure_run(measure_op_t op, uint32_t *counts, size_t n)
{
	if (counts == NULL || n == 0u) {
		return -EINVAL;
	}

	switch (op) {
	case MEASURE_OP_YIELD_EQUAL:
		run_yield(counts, n);
		return 0;
	case MEASURE_OP_BLOCK_ONE:
		run_block_one(counts, n);
		return 0;
	case MEASURE_OP_BLOCK_SEVERAL:
		run_block_several(counts, n);
		return 0;
	case MEASURE_OP_HAND_TO_QUEUE:
		run_hand_to_queue(counts, n);
		return 0;
	case MEASURE_OP_PERIOD:
		return run_period(counts, n);
	case MEASURE_OP_MUTEX_INHERIT_ON:
		mutex_case_run(true, counts, n);
		return 0;
	case MEASURE_OP_MUTEX_INHERIT_OFF:
		mutex_case_run(false, counts, n);
		return 0;
	default:
		return -EINVAL;
	}
}

/* ---- creation, before the scheduler starts ----------------------------------------------- */

bool measure_adapter_init(void)
{
	go = xSemaphoreCreateBinaryStatic(&go_storage);
	signalled = xSemaphoreCreateBinaryStatic(&signalled_storage);
	work_done = xSemaphoreCreateBinaryStatic(&work_done_storage);
	events = xEventGroupCreateStatic(&events_storage);
	if (go == NULL || signalled == NULL || work_done == NULL || events == NULL) {
		return false;
	}

	/* Created, then suspended before the scheduler has started, so that it has never run
	 * when the first row that is not the yield row needs the giver to be schedulable. The
	 * kernel resets its notion of the current task when the task it pointed at is suspended
	 * before the scheduler runs, and the first host build on Saturday 17 October 2026 showed
	 * it does so without complaint. */
	partner_task = xTaskCreateStatic(partner_fn, "partner", STACK_WORDS, NULL, PRIO_PARTNER,
					 partner_stack, &partner_control);
	if (partner_task == NULL) {
		return false;
	}
	vTaskSuspend(partner_task);

	if (xTaskCreateStatic(giver_fn, "giver", STACK_WORDS, NULL, PRIO_GIVER, giver_stack,
			      &giver_control) == NULL) {
		return false;
	}

	measure_port_marker_init();

	return mutex_case_init();
}
