/* projects/04-what-the-kernel-primitives-cost/freertos/board_main.c: what only FreeRTOS on the
 * NUCLEO-H7A3ZI-Q needs.
 *
 * The campaign is the same one the Zephyr build and the host build run, in
 * ../adapter/campaign.c. What this file supplies is what that campaign cannot know: how this
 * kernel spins and sleeps for the two clock gates, what the hardware and the build say about
 * caches and configuration, how the marker is driven, and how the part is brought to 280 MHz
 * before a kernel configured for 280 MHz is allowed to start.
 *
 * THE BOARD SUPPORT IS THE SIBLING VOLUME'S, at a pinned commit, not copied. Its startup.c
 * carries the vector table with the three handlers FreeRTOS replaces by name, its linker script
 * places everything in DTCM, its clock280.c climbs to 280 MHz one gated step at a time and
 * refuses rather than guessing, and its retarget.c is what printf reaches the console through.
 * Every one of those has run on this board since the first week of October 2026.
 *
 * ORDER OF START-UP, AND WHY EACH STEP WAITS FOR THE ONE BEFORE. board_init() brings up the
 * pins, the cycle counter and the console at the reset clock. The climb to 280 MHz changes the
 * bus clock the console's divider was computed from, so the console is initialised again
 * afterwards; between the two it prints nothing. The decode is then checked against the rate
 * the kernel was configured for, and if they disagree the scheduler is never started, because a
 * tick configured for one rate on a part running at another would make every wall figure wrong
 * by a factor while the console read fine. Only then are the cache enabled, the marker pin
 * configured, the tasks created and the scheduler started.
 */
#include "../adapter/campaign.h"
#include "../adapter/measure.h"
#include "port.h"

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "stm32h7a3_regs.h"

#include <stdio.h>

bool measure_adapter_init(void);

/* ---------------------------------------------------- the kernel's hooks into the application */

/* A failed assertion prints where and spins. Nothing to exit to on a board, and spinning
 * leaves the frame for a debugger. */
void p04_freertos_assert(const char *file, unsigned long line)
{
	printf("# FAIL kernel assertion at %s:%lu\n", file, line);
	for (;;) {
		__asm volatile ("nop");
	}
}

/* configCHECK_FOR_STACK_OVERFLOW is 2, so an overflow found at a switch arrives here with the
 * task's name, which is worth more than a hard fault with none. */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
	(void)task;
	printf("# FAIL stack overflow in task %s\n", name);
	for (;;) {
		__asm volatile ("nop");
	}
}

/* V10.4.4 declares these two hooks with uint32_t and V11.1.0 with configSTACK_DEPTH_TYPE,
 * so an application that provides them cannot compile against both without saying which.
 * Found on Monday 26 October 2026 by the run that measures this kernel's own two versions
 * against each other; it is the only difference that stood in the way. The pinned build is
 * byte for byte what it was without this block, 27148 bytes, which is the control that says
 * the guard changes nothing for the version this project publishes figures against.
 * tskKERNEL_VERSION_MAJOR comes from task.h, included above. */
#if (tskKERNEL_VERSION_MAJOR >= 11)
#define BENCH_STACK_DEPTH_T configSTACK_DEPTH_TYPE
#else
#define BENCH_STACK_DEPTH_T uint32_t
#endif

static StaticTask_t idle_control;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxTaskTCB, StackType_t **ppxTaskStack,
				   BENCH_STACK_DEPTH_T *puxTaskStackSize)
{
	*ppxTaskTCB = &idle_control;
	*ppxTaskStack = idle_stack;
	*puxTaskStackSize = (BENCH_STACK_DEPTH_T)configMINIMAL_STACK_SIZE;
}

static StaticTask_t timer_control;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB, StackType_t **ppxTimerStack,
				    BENCH_STACK_DEPTH_T *puxTimerStackSize)
{
	*ppxTimerTCB = &timer_control;
	*ppxTimerStack = timer_stack;
	*puxTimerStackSize = (BENCH_STACK_DEPTH_T)configTIMER_TASK_STACK_DEPTH;
}

/* ------------------------------------------------------------------ the campaign's hooks */

#define CLOCK_CHECK_MS 200u

/* THE CORE GATE SPINS, because its counter stops when the core idles. The window is set by
 * the tick count, which is the kernel's millisecond clock off the system timer and therefore
 * neither of the two cycle counters: a window set by the counter under test would be
 * self-referential, and a gate that cannot fail has shown nothing. The Zephyr adapter reached
 * this shape on Friday 9 October 2026 through a refusal of 4260 counts for a 200 ms sleep. */
static bool core_clock_agrees(void)
{
	uint32_t hz = measure_clock_hz();
	uint32_t predicted, elapsed, t0;
	TickType_t start;

	if (hz == 0u) {
		return false;
	}
	predicted = (hz / 1000u) * CLOCK_CHECK_MS;

	start = xTaskGetTickCount();
	t0 = measure_now();
	while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(CLOCK_CHECK_MS)) {
		/* Spinning on purpose. An idle core is a stopped instrument. */
	}
	elapsed = measure_now() - t0;

	printf("# clock check: %u ms of spinning advanced the counter by %u, and %u Hz\n",
	       (unsigned int)CLOCK_CHECK_MS, (unsigned int)elapsed, (unsigned int)hz);
	printf("#   predicts %u, so the count is %u per cent of the prediction\n",
	       (unsigned int)predicted,
	       (unsigned int)(((uint64_t)elapsed * 100u) / predicted));

	return elapsed > (predicted - predicted / 4u) && elapsed < (predicted + predicted / 4u);
}

/* THE WALL GATE SLEEPS, because the wall counter exists to run through idle, and if it cannot
 * time a sleep to within a quarter it cannot time the period row either. */
static bool wall_clock_agrees(void)
{
	uint32_t hz = measure_clock_hz_wall();
	uint32_t predicted, elapsed, t0;

	if (hz == 0u) {
		return false;
	}
	predicted = (hz / 1000u) * CLOCK_CHECK_MS;

	t0 = measure_now_wall();
	vTaskDelay(pdMS_TO_TICKS(CLOCK_CHECK_MS));
	elapsed = measure_now_wall() - t0;

	printf("# wall clock check: %u ms of SLEEP advanced the wall counter by %u, and %u Hz\n",
	       (unsigned int)CLOCK_CHECK_MS, (unsigned int)elapsed, (unsigned int)hz);
	printf("#   predicts %u, so the count is %u per cent of the prediction\n",
	       (unsigned int)predicted,
	       (unsigned int)(((uint64_t)elapsed * 100u) / predicted));

	return elapsed > (predicted - predicted / 4u) && elapsed < (predicted + predicted / 4u);
}

/* What the hardware and the build say, asked rather than assumed, in the same shape as the
 * Zephyr adapter's report so the two consoles read side by side. */
static void report(void)
{
	printf("# clock, as the registers decode it after the climb\n");
	printf("#   core %u Hz, APB1 %u Hz, the kernel configured for %lu Hz\n",
	       (unsigned int)board_core_hz(), (unsigned int)board_pclk1_hz(),
	       (unsigned long)configCPU_CLOCK_HZ);
	printf("# caches, as the hardware reports\n");
	printf("#   the control register says instruction cache %s, data cache %s\n",
	       (SCB_CCR & (1u << SCB_CCR_IC_POS)) != 0u ? "ON" : "OFF",
	       (SCB_CCR & (1u << SCB_CCR_DC_POS)) != 0u ? "ON" : "OFF");
	printf("# configuration that bears on the comparison with the Zephyr build\n");
	printf("#   configUSE_PREEMPTION %d, configUSE_TIME_SLICING %d, configUSE_MUTEXES %d\n",
	       configUSE_PREEMPTION, configUSE_TIME_SLICING, configUSE_MUTEXES);
	printf("#   configTICK_RATE_HZ %lu, configCHECK_FOR_STACK_OVERFLOW %d\n",
	       (unsigned long)configTICK_RATE_HZ, configCHECK_FOR_STACK_OVERFLOW);
	printf("#   timer daemon at priority %d above the measurer at %d\n",
	       (int)configTIMER_TASK_PRIORITY, (int)(tskIDLE_PRIORITY + 3));
	printf("#   core instrument, six rows: the cycle counter, which stops when the core does\n");
	printf("#   wall instrument, the period row: ticks and the SysTick count between them\n");
	printf("#   marker for the witness: pin 4 on gpio@58020400, %s\n",
	       measure_port_marker_ready() ? "configured as an output, one toggle per period"
					   : "NOT CONFIGURED, so the period row will be withheld");
}

/* ---- the marker, in the same pattern the Zephyr adapter drives ---------------------------- */

#define BEACON_EDGES 8u
#define BEACON_MS    250u

static void marker_beacon(void)
{
	unsigned int i;

	if (!measure_port_marker_ready()) {
		return;
	}
	printf("# marker beacon: %u edges %u ms apart, about %u ms of slow square wave,\n",
	       (unsigned int)BEACON_EDGES, (unsigned int)BEACON_MS,
	       (unsigned int)(BEACON_EDGES * BEACON_MS));
	printf("#   so a witness recording that overlaps this boot at all contains it\n");
	for (i = 0u; i < BEACON_EDGES; i++) {
		vTaskDelay(pdMS_TO_TICKS(BEACON_MS));
		measure_port_marker_toggle();
	}
}

/* The resting pattern: a gap, then the same sixty-four sleeps the period row brackets,
 * forever. A ten second recording started at any later moment holds several whole bursts, and
 * witness.py reports whether they agree with each other. The level a burst starts from does
 * not matter, since edges in either direction are what the witness counts, which is why there
 * is no set() before the gap: the Zephyr adapter had one and it fired an edge microseconds
 * after the last toggle, which the witness merged. */
static void marker_rest(void)
{
	unsigned int i;

	if (!measure_port_marker_ready()) {
		printf("\n# no marker, so no resting pattern. The pin could not be configured.\n");
		for (;;) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
	printf("\n# resting pattern: a gap, then the same %u periods this row measured,\n",
	       (unsigned int)MEASURE_SAMPLES);
	printf("#   repeating for as long as this board is powered. A witness recording\n");
	printf("#   started at any later moment contains several whole bursts.\n");

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1500));
		measure_port_marker_toggle();
		for (i = 0u; i < MEASURE_SAMPLES; i++) {
			vTaskDelay(pdMS_TO_TICKS(1));
			measure_port_marker_toggle();
		}
	}
}

/* ------------------------------------------------------------------------------ the run */

static void measurer_fn(void *arg)
{
	static const measure_campaign_t campaign = {
		.kernel = "freertos",
		.built = __DATE__ " " __TIME__,
		.report = report,
		.core_clock_agrees = core_clock_agrees,
		.wall_clock_agrees = wall_clock_agrees,
		.set_cold = measure_port_set_cold,
		.marker_beacon = marker_beacon,
		.marker_rest = marker_rest,
	};
	int status;

	(void)arg;
	status = measure_campaign_run(&campaign);

	/* Reached only when the campaign refused or a verdict failed, since otherwise the
	 * resting pattern never returns. Say so, and stay. */
	printf("# the campaign returned %d and the board is idle\n", status);
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

#define MEASURER_STACK_WORDS (configMINIMAL_STACK_SIZE * 4)
static StaticTask_t measurer_control;
static StackType_t measurer_stack[MEASURER_STACK_WORDS];

static void halt(const char *why)
{
	printf("# REFUSED %s. The scheduler is not started.\n", why);
	for (;;) {
		__asm volatile ("nop");
	}
}

int main(void)
{
	clock280_result_t climb;

	board_init();

	/* THE CLIMB, THEN THE CONSOLE AGAIN. board_clock_raise_to_280() is the sibling volume's
	 * gated sequence and refuses on any step rather than guessing; the rescan decodes the
	 * registers afterwards rather than trusting the writes; and the console's divider is
	 * recomputed from the decoded bus clock, which board_console_init() does on every call.
	 * Nothing is printed between the climb and the re-initialisation. */
	(void)board_clock_raise_to_280(&climb);
	board_clock_rescan();
	board_console_init();

	if (!climb.reached_280 || board_core_hz() != (uint32_t)configCPU_CLOCK_HZ) {
		printf("# the climb to 280 MHz failed at step %s; the registers decode to %u Hz\n",
		       board_clock280_step_name(climb.failed_at), (unsigned int)board_core_hz());
		halt("the part is not running at the rate the kernel was configured for");
	}

	/* The instruction cache, so that criterion 3 has two states to compare. board_init()
	 * leaves it alone deliberately; this application wants it on, as the Zephyr build has
	 * it, and says what it found. */
	if (!board_icache_enable()) {
		printf("# the instruction cache could not be enabled\n");
	}

	if (!board_cycles_available()) {
		halt("the cycle counter did not advance in its own check");
	}

	if (!measure_adapter_init()) {
		halt("the adapter could not create its static objects");
	}

	if (xTaskCreateStatic(measurer_fn, "measurer", MEASURER_STACK_WORDS, NULL,
			      tskIDLE_PRIORITY + 3, measurer_stack, &measurer_control) == NULL) {
		halt("the measurer task could not be created");
	}

	vTaskStartScheduler();

	halt("the scheduler returned");
	return 1;
}
