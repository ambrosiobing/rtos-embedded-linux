/* projects/01-presence/freertos/FreeRTOSConfig.h
 *
 * One config for the host build. The board build will need its own, because the
 * clock rate, the stack sizes and the tick source are the three things that are
 * genuinely different, and pretending one file covers both is how a host result gets
 * reported as a board result.
 *
 * THE LINE THAT MATTERS IS configSUPPORT_DYNAMIC_ALLOCATION AT 0. With it at zero,
 * xQueueCreate, xTimerCreate and xTaskCreate are not compiled at all, so an
 * accidental call to one is a link error rather than something to find later in a
 * map file. That is the stronger form of "no allocator is linked into this
 * application": not a measurement afterwards, but a build that cannot express it.
 * The cost is that the application must hand FreeRTOS the memory for its own idle
 * and timer tasks, which host_main.c does.
 *
 * WHAT HAS NOT BEEN CONFIRMED. The POSIX port is a real port with real requirements
 * and this file is written from its documented ones. The first build in WSL on the
 * demo laptop is what settles whether it wants anything else, and that build is the
 * first test rather than CI, because a red run says less and costs more.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ----------------------------------------------------------- the scheduler */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_16_BIT_TICKS                  0
#define configMAX_PRIORITIES                    5
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/* The POSIX port gives every task a pthread, so a "stack depth" here is a request
 * to pthread_attr_setstacksize rather than an array this project owns. It is set
 * well above PTHREAD_STACK_MIN on purpose: a host thread stack is cheap, and the
 * number that matters for the board is the high-water mark on the board. */
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 4096 )

/* ------------------------------------------------------------- allocation */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        0
#define configUSE_MALLOC_FAILED_HOOK            0

/* ---------------------------------------------------------------- timers */
/* Both of this project's timers are software timers, so the timer service task is
 * not optional here. Its priority is the highest, because an expiry that waits
 * behind the dispatcher would make at_ms later than the moment the hold was due,
 * and at_ms is what the guard on row 17 compares. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* ------------------------------------------------------------ what is unused */
/* Named rather than omitted, so that a reader can see the surface this application
 * actually uses. Four states, six events, one queue and two timers need very little
 * of a kernel, which is itself one of the findings of comparing three of them. */
#define configUSE_MUTEXES                       0
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_TRACE_FACILITY                0
#define configGENERATE_RUN_TIME_STATS           0
#define configQUEUE_REGISTRY_SIZE               0

/* Stack overflow checking is off and that is not laziness. On the POSIX port a task
 * runs on a pthread stack, so the canary FreeRTOS would watch is not the stack the
 * code is actually using and a pass here would mean nothing. The board build turns
 * it on at 2, where it does mean something. */
#define configCHECK_FOR_STACK_OVERFLOW          0

/* --------------------------------------------------------------- the API used */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_uxTaskGetStackHighWaterMark     0

/* A failed assertion stops the run and says where. Silence here would turn a kernel
 * misuse into a hang, and a hang in CI is six minutes of nothing followed by a
 * timeout nobody can diagnose. */
extern void presence_freertos_assert(const char *file, unsigned long line);
#define configASSERT( x ) \
    if ( ( x ) == 0 ) { presence_freertos_assert( __FILE__, __LINE__ ); }

#endif /* FREERTOS_CONFIG_H */
