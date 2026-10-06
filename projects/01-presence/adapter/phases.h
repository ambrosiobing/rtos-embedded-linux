/* projects/01-presence/adapter/phases.h: the same four phases, under every kernel.
 *
 * One test file, several adapters. Each kernel's main starts a thread that calls
 * presence_phases_run and exits with what it returns, so the claim is not "each
 * adapter passes its own test" but "one test passes against each adapter", which is
 * the only form of that claim worth making.
 */
#ifndef PRESENCE_PHASES_H
#define PRESENCE_PHASES_H

/* Runs all four phases in order and returns the number of failures. Prints a line per
 * phase and a verdict at the end. Must be called from a thread, after
 * presence_adapter_init and with the scheduler running, because every phase posts
 * events and waits for the dispatch thread to take them. */
int presence_phases_run(void);

#endif /* PRESENCE_PHASES_H */
