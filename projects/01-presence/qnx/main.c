/* projects/01-presence/qnx/main.c: what only QNX needs, which is almost nothing.
 *
 * **NEVER COMPILED.** No QNX licence and no QNX target on this bench. See the header
 * comment of presence_adapter.c, which says the same thing at greater length and
 * explains why the directory exists anyway.
 *
 * There is no build file here, deliberately. A Makefile or a .mk for a toolchain
 * nobody here has would be a guess dressed as an instruction, and this project has
 * one rule it keeps returning to: say which evidence a claim rests on. The two
 * adapters that can be built carry build files that have been run.
 */
#include "phases.h"
#include "presence_adapter.h"

#include <pthread.h>
#include <sched.h>
#include <stdio.h>

/* The lamps. Three GPIOs on a board, and on QNX most likely a resource manager rather
 * than a direct register write, which is a difference that belongs to the application
 * and not to any adapter. Nothing here, so that a line per dispatch does not bury the
 * phases. */
void presence_indicators(presence_state_t state)
{
    (void)state;
}

int main(void)
{
    struct sched_param sp;

    printf("the presence table under QNX\n\n");

    /* Below the dispatch thread, which is what makes a post deterministic. QNX counts
     * priorities upwards, so below is the smaller number, which is the opposite of
     * Zephyr and the same as FreeRTOS. The adapter owns both values. */
    sp.sched_priority = 19;
    (void)pthread_setschedparam(pthread_self(), SCHED_RR, &sp);

    if (!presence_adapter_init()) {
        printf("  FAIL the adapter could not create its channel, timers or thread\n");
        return 1;
    }

    return presence_phases_run();
}
