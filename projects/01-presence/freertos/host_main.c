/* projects/01-presence/freertos/host_main.c: what only FreeRTOS needs.
 *
 * The four test phases are not here. They are in ../adapter/phases.c, which includes
 * no kernel header and runs unchanged against every adapter, so the claim is "one
 * test passes against each kernel" rather than "each kernel passes a test of its
 * own". What is left in this file is the three things a FreeRTOS host build has to
 * supply and no other kernel does: the memory its own idle and timer tasks need, a
 * place for a failed kernel assertion to be seen, and a thread to run the phases on.
 */
#include "phases.h"
#include "presence_adapter.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

/* The lamps. On the board these are three GPIOs; here the state is not printed,
 * because a line per dispatch would bury the phases. The adapter cannot tell the
 * difference, which is the point of the hook being the application's. */
void presence_indicators(presence_state_t state)
{
    (void)state;
}

/* A failed kernel assertion stops the run and says where. Silence here would turn a
 * misuse of the kernel into a hang, and a hang in CI is six minutes of nothing
 * followed by a timeout nobody can diagnose. */
void presence_freertos_assert(const char *file, unsigned long line)
{
    printf("  FAIL kernel assertion at %s:%lu\n", file, line);
    fflush(stdout);
    exit(2);
}

/* configSUPPORT_DYNAMIC_ALLOCATION is 0, so FreeRTOS cannot allocate the memory for
 * its own idle and timer tasks and asks the application for it. That is the cost of
 * making an accidental dynamic create a link error, and it is worth paying.
 *
 * The third parameter is configSTACK_DEPTH_TYPE and not uint32_t. It defaults to
 * StackType_t, an unsigned long here, so the difference is one of width and the first
 * build said so. Written as the kernel spells it, so a port where StackType_t is
 * sixteen bits does not quietly disagree either. */
static StaticTask_t idle_control;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxTaskTCB,
                                   StackType_t **ppxTaskStack,
                                   configSTACK_DEPTH_TYPE *puxTaskStackSize)
{
    *ppxTaskTCB = &idle_control;
    *ppxTaskStack = idle_stack;
    *puxTaskStackSize = (configSTACK_DEPTH_TYPE)configMINIMAL_STACK_SIZE;
}

static StaticTask_t timer_control;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB,
                                    StackType_t **ppxTimerStack,
                                    configSTACK_DEPTH_TYPE *puxTimerStackSize)
{
    *ppxTimerTCB = &timer_control;
    *ppxTimerStack = timer_stack;
    *puxTimerStackSize = (configSTACK_DEPTH_TYPE)configTIMER_TASK_STACK_DEPTH;
}

/* ---------------------------------------------------------------------- the run */

static void test_task(void *arg)
{
    (void)arg;
    exit(presence_phases_run());
}

#define TEST_STACK_WORDS ( configMINIMAL_STACK_SIZE )
static StaticTask_t test_control;
static StackType_t test_stack[TEST_STACK_WORDS];

int main(void)
{
    printf("the presence table under FreeRTOS, on a host\n\n");

    if (!presence_adapter_init()) {
        printf("  FAIL the adapter could not create its static objects\n");
        return 1;
    }

    /* One below the dispatch thread, which is what makes a post deterministic: the
     * dispatcher preempts this one the moment an event is queued. The phase that
     * fills the queue raises itself above the dispatcher and back, through the
     * adapter, which is the only place that knows either priority. */
    if (xTaskCreateStatic(test_task, "test", TEST_STACK_WORDS, NULL,
                          tskIDLE_PRIORITY + 1, test_stack,
                          &test_control) == NULL) {
        printf("  FAIL the test task could not be created\n");
        return 1;
    }

    vTaskStartScheduler();

    /* Only reached if the scheduler returns, which it should not. */
    printf("  FAIL the scheduler returned\n");
    return 1;
}
