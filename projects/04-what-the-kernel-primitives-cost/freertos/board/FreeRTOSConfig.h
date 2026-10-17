/* projects/04-what-the-kernel-primitives-cost/freertos/board/FreeRTOSConfig.h
 *
 * The board's configuration, kept apart from the host's because the three things that
 * genuinely differ, the clock rate, the stack sizes and the tick source, are the three things
 * a shared file would get wrong for one of them. It sits in its own directory so that the
 * board build finds this one first and the host build never sees it.
 *
 * THE SAME TWO LINES MATTER HERE AS ON THE HOST. configSUPPORT_DYNAMIC_ALLOCATION at 0, so an
 * allocating create is a link error and the Makefile checks the binary for pvPortMalloc.
 * configUSE_TIME_SLICING at 0, stated rather than defaulted, because the yield row runs two
 * tasks at equal priority and a slice boundary inside a bracket would add a switch the
 * bracket did not ask for; the Zephyr adapter ran a day with the equivalent on by default.
 *
 * THE CLOCK RATE IS A CLAIM THIS FILE CANNOT CHECK, so the main does: it climbs to 280 MHz
 * through the sibling volume's gated sequence, decodes the registers, and refuses to start
 * the scheduler if the decode disagrees with the figure below. A kernel whose tick was
 * configured for a rate the part is not running at would make every wall figure wrong by a
 * factor while the console looked fine, which is exactly the shape of error this project's
 * gates exist for.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ----------------------------------------------------------- the scheduler */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_16_BIT_TICKS                  0
#define configCPU_CLOCK_HZ                      ( 280000000UL )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/* Five levels, as the host: idle 0, giver and holder 1, medium 2, measurer and partner 3,
 * timer daemon 4. */
#define configMAX_PRIORITIES                    5

/* In WORDS. 256 words is a kilobyte, which is plenty for tasks that take a semaphore and
 * spin; the measurer gets four times that because it is the one that prints, and newlib's
 * printf uses stack. Everything is static and lands in DTCM, where the linker script keeps
 * eight kilobytes for the main stack beside it. */
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 256 )

/* ------------------------------------------------------------- allocation */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        0
#define configUSE_MALLOC_FAILED_HOOK            0

/* ---------------------------------------------------------------- timers */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* ------------------------------------------------------------ what is used */
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

/* ON, AT 2, where it means something: on the board a task runs on the stack this file sized,
 * and the kernel checks the canary at every switch. The host leaves this off because its tasks
 * run on pthread stacks the canary is not watching. */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* --------------------------------------------- the interrupt priorities, Cortex-M7 */
/* This part implements four priority bits, the top four of each eight-bit field. The kernel's
 * own interrupts run at the lowest priority, and any interrupt that calls an ISR-safe kernel
 * function must sit at or below the syscall ceiling. Nothing in this application raises an
 * interrupt of its own, so the only interrupts that exist are the kernel's three. */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY \
	( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* THE THREE HANDLERS, ROUTED BY NAME. The sibling volume's startup.c declares SVC_Handler,
 * PendSV_Handler and SysTick_Handler as weak aliases of a spin loop, so the port's own
 * functions, compiled under these names, replace them in the vector table with no wrapper in
 * between. A wrapper would be wrong: two of the three are naked assembly that reads the
 * stack frame the hardware pushed, and a C call in front of them would push another. The
 * port checks at start-up that the table really does point at them. */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler
#define configCHECK_HANDLER_INSTALLATION        1

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

/* A failed assertion prints where and then spins, so the console shows it and a debugger
 * finds the frame. On a board there is nothing to exit to. */
extern void p04_freertos_assert(const char *file, unsigned long line);
#define configASSERT( x ) \
    if ( ( x ) == 0 ) { p04_freertos_assert( __FILE__, __LINE__ ); }

#endif /* FREERTOS_CONFIG_H */
