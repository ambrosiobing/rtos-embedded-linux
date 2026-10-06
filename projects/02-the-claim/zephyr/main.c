/* projects/02-the-claim/zephyr/main.c: what only Zephyr needs.
 *
 * Which is very little, and that is the finding, repeated from P01 because it holds for a
 * second application. The FreeRTOS main has to hand the kernel the memory for its own idle
 * and timer tasks, catch a failed assertion, and create a thread for the phases before
 * starting a scheduler. Zephyr's main IS a thread and the scheduler is already running, so
 * this file supplies the lamps, a way to leave with a status, and nothing else.
 */
#include "claim_adapter.h"
#include "phases.h"

#include <zephyr/kernel.h>

#include <stdio.h>

/* Leaving the simulation with an exit status, which is the one thing here that is
 * native_sim's business rather than Zephyr's. The header that provides it has been renamed
 * across releases, so it is selected rather than assumed, and the libc exit is the fallback.
 * If a future release renames it again this is where to look. */
#if defined(__has_include)
#  if __has_include(<nsi_main.h>)
#    include <nsi_main.h>
#    define CLAIM_EXIT(code) nsi_exit(code)
#  elif __has_include(<posix_board_if.h>)
#    include <posix_board_if.h>
#    define CLAIM_EXIT(code) posix_exit(code)
#  endif
#endif
#ifndef CLAIM_EXIT
#  include <stdlib.h>
#  define CLAIM_EXIT(code) exit(code)
#endif

/* The lamps, which in this chapter show the CLAIM and not the presence. Three GPIOs on the
 * board; nothing here, because a line per dispatch would bury the phases. The adapter
 * cannot tell the difference, which is why the hook belongs to the application. */
void claim_indicators(claim_code_t code)
{
    ARG_UNUSED(code);
}

int main(void)
{
    int failures;

    printf("the claim cascade under Zephyr, on native_sim\n\n");

    /* Below the dispatch thread, which is what makes a post deterministic. Zephyr counts
     * priorities downwards, so "below" is the larger number; the adapter owns both values
     * and the phase that fills the queue flips this through it. */
    k_thread_priority_set(k_current_get(), 6);

    if (!claim_adapter_init()) {
        printf("  FAIL the adapter could not create its static objects\n");
        CLAIM_EXIT(1);
    }

    failures = claim_phases_run();
    CLAIM_EXIT(failures);

    return failures;
}
