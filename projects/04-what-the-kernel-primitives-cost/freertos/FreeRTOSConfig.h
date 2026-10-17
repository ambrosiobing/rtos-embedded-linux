/* projects/04-what-the-kernel-primitives-cost/freertos/FreeRTOSConfig.h
 *
 * One config for the host build, on the POSIX port. The board build will need its own,
 * because the clock rate, the stack sizes and the tick source are the three things that are
 * genuinely different, and pretending one file covers both is how a host result gets reported
 * as a board result. This project cannot make that mistake by accident in any case: the host
 * port reports an unconfirmed clock and the reduction refuses everything it emits.
 *
 * THE LINE THAT MATTERS IS configSUPPORT_DYNAMIC_ALLOCATION AT 0, as in P01 and P02. With it
 * at zero, every allocating create is a link error rather than something to find later in a
 * map file, and the Makefile checks the binary for pvPortMalloc afterwards anyway.
 *
 * THE SECOND LINE THAT MATTERS IS configUSE_TIME_SLICING AT 0, and it is specific to this
 * project. The Zephyr adapter was built with CONFIG_TIMESLICING on by default and nobody had
 * read the default; it was matched to Zephyr's own benchmark on Friday 9 October 2026. Time
 * slicing round-robins equal-priority tasks on the tick, and the yield row runs two tasks at
 * equal priority, so a slice boundary inside a bracket would add a switch the bracket did not
 * ask for. Off, and stated, rather than on by default and discovered.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ----------------------------------------------------------- the scheduler */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_16_BIT_TICKS                  0
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/* Five levels, and the adapter uses four of them: the idle task at 0, the giver and the
 * lock holder at 1, the medium task at 2, the measurer and its yield partner at 3, and the
 * timer daemon at 4 so that handing work to it preempts straight into the handler. */
#define configMAX_PRIORITIES                    5

/* The POSIX port gives every task a pthread, so a "stack depth" here is a request to
 * pthread_attr_setstacksize rather than an array this project owns. Well above
 * PTHREAD_STACK_MIN on purpose; the number that matters is the board's high-water mark. */
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 4096 )

/* ------------------------------------------------------------- allocation */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        0
#define configUSE_MALLOC_FAILED_HOOK            0

/* ---------------------------------------------------------------- timers */
/* No software timer is armed anywhere in this project. The timer service task is here for
 * one reason: xTimerPendFunctionCall() runs a function on it, and that is FreeRTOS's
 * counterpart to Zephyr's system work queue for the hand-work-to-a-queue row. Its priority is
 * the highest so the post preempts into the handler, which is what makes that bracket hold
 * one switch rather than a round trip, exactly as the Zephyr adapter's does. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* ------------------------------------------------------------ what is used */
/* Mutexes, because the inheriting arm of criterion 4 is a FreeRTOS mutex, which inherits
 * priority and cannot be told not to. The non-inheriting arm is a binary semaphore used as a
 * lock, so the comparison is between two protocols and not two settings of one object. Event
 * groups, because the several-objects row waits on one. Counting semaphores, queue sets and
 * task notifications are not used and are named as such rather than omitted. */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TASK_NOTIFICATIONS            0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_TRACE_FACILITY                0
#define configGENERATE_RUN_TIME_STATS           0
#define configQUEUE_REGISTRY_SIZE               0

/* Off on the POSIX port, where a task runs on a pthread stack the canary is not watching. The
 * board build turns it on at 2, where it means something. */
#define configCHECK_FOR_STACK_OVERFLOW          0

/* --------------------------------------------------------------- the API used */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_uxTaskGetStackHighWaterMark     0

/* A failed assertion stops the run and says where. Silence here would turn a kernel misuse
 * into a hang, and a hang in CI is six minutes of nothing followed by a timeout. */
extern void p04_freertos_assert(const char *file, unsigned long line);
#define configASSERT( x ) \
    if ( ( x ) == 0 ) { p04_freertos_assert( __FILE__, __LINE__ ); }

#endif /* FREERTOS_CONFIG_H */
