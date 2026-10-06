/* projects/01-presence/adapter/presence_adapter.h: one contract, three kernels.
 *
 * This header is shared by every adapter. `../freertos/`, `../zephyr/` and `../qnx/`
 * each implement exactly this and nothing else, which is the same arrangement the
 * table itself has in C, C++ and Rust: one specification, several implementations,
 * and the comparison is only meaningful because the surface is identical.
 *
 * IT USED TO LIVE IN ../freertos/, AND WRITING THE SECOND ADAPTER IS WHAT MOVED IT.
 * The first version of `presence_adapter_post_from_isr` took a
 * `long *higher_priority_task_woken`, which is FreeRTOS's convention for telling the
 * caller to yield. Zephyr's `k_msgq_put` needs no such thing and QNX's interrupt
 * handling needs something different again, so that parameter was never part of the
 * contract: it was one kernel's calling convention written into a header three
 * kernels have to satisfy. It is gone, and each adapter yields on its own behalf,
 * which is where that decision belonged. One implementation cannot tell you which
 * parts of an interface are general. Two can.
 *
 * WHAT AN ADAPTER IS. It carries events and decides nothing. docs/RTOS_VARIANTS.md
 * states it as a negative, which is the useful form: no comparison against a range,
 * no count of readings, no notion of how long a hold lasts, and no opinion about what
 * an event means. A threshold found in an adapter means the adapter is wrong, not the
 * table. Every number lives in presence_settings_t and every decision is a row.
 *
 * THE THREE THINGS THE TABLE NEEDS BACK, which every implementation must provide:
 *
 *   1. One queue, one consumer, first in first out. Events are dispatched in the
 *      order they were posted. Every kernel here offers a call that puts an event at
 *      the head of the queue instead, and no adapter may use one: rows 4, 11 and 18
 *      exist because order is what makes a stale expiry harmless.
 *   2. at_ms stamped when an event is posted, never when it is dispatched. An adapter
 *      that filled it in at the front of the dispatch loop would make every stale
 *      event look fresh and defeat the guard on row 17.
 *   3. A cancelled hold may still deliver its expiry. No kernel can un-queue a timer
 *      callback that has already fired, and the table absorbs that rather than the
 *      adapter dropping anything.
 *
 * THE ONE PIECE OF STATE AN ADAPTER READS is hold_running, and only to mirror it onto
 * the kernel timer: false to true arms, true to false cancels. It is read after
 * presence_dispatch returns and never written.
 */
#ifndef PRESENCE_ADAPTER_H
#define PRESENCE_ADAPTER_H

#include "presence.h"

#include <stdbool.h>
#include <stdint.h>

/* Create the queue, the two timers and the dispatch thread, all statically, and start
 * nothing. Call before the scheduler runs, where a kernel has that distinction.
 * Returns false if any object could not be created, which under static allocation
 * means a programming error rather than exhaustion. */
bool presence_adapter_init(void);

/* Start the heartbeat, which posts PRESENCE_EV_TICK every period_ms for ever.
 *
 * Separate from init because of the test rather than the board: a periodic event
 * arriving in the middle of a scripted sequence would interleave TICK rows with the
 * rows under test. The phases run with the heartbeat stopped and then start it
 * themselves, which is also the only way to show that it works. */
bool presence_adapter_start_tick(uint32_t period_ms);

/* Post an event from a thread. Returns false when the queue is full, which is an
 * event lost, which is how a release goes missing: the count is kept and
 * presence_adapter_events_dropped reports it. Never blocks. */
bool presence_adapter_post(const presence_event_t *ev);

/* The same from an interrupt. Each adapter yields on its own behalf if its kernel
 * needs that, because how a kernel is told to reschedule is that kernel's business
 * and not this contract's. Nothing calls this on a host build; it exists because on
 * the board the button and the sensor both arrive this way. */
bool presence_adapter_post_from_isr(const presence_event_t *ev);

/* How many posts were refused because the queue was full. A dropped event cannot be
 * recovered, so the only honest thing is to count it and report it. Zero is the only
 * acceptable value in a run that claims to have measured anything. */
uint32_t presence_adapter_events_dropped(void);

/* Read-only access to the machine, for a shell command or a test. const, because
 * nothing outside the dispatch thread may touch it. */
const presence_t *presence_adapter_context(void);

/* Called after every dispatch, with the row taken and the context after it.
 * Observability and not a decision: the adapter does not look at what the hook does,
 * and the hook may not post events. The host test uses it to compare the row sequence
 * through a real queue against the sequence the direct-dispatch test produces. */
typedef void (*presence_trace_fn)(uint8_t row, const presence_t *after);
void presence_adapter_set_trace(presence_trace_fn fn);

/* The three indicators, as a function of state. Defined by the application, once per
 * board, and deliberately not by an adapter: a lamp is the one thing that differs
 * between a Nucleo and a host, and an adapter that knew about a GPIO would be a third
 * thing to port. */
void presence_indicators(presence_state_t state);

/* ------------------------------------------------- what the shared phases need
 *
 * The four test phases in ../adapter/phases.c run unchanged under every kernel, which
 * is the strongest form the claim can take: not "each adapter passes its own test"
 * but "one test passes against each adapter". These two are the only kernel-specific
 * things those phases need, so each adapter supplies them and the phases stay free of
 * any kernel's header.
 */

/* Sleep the calling thread. */
void presence_adapter_sleep_ms(uint32_t ms);

/* Raise the calling thread above the dispatch thread, or put it back below.
 *
 * Needed by one phase only, and that phase is the reason this exists: to fill the
 * queue on purpose, nothing may drain it, so the producer has to outrank the
 * consumer for as long as it takes. Without it the queue never fills and the phase
 * passes by never testing anything. */
void presence_adapter_test_outrank_dispatcher(bool outrank);

#endif /* PRESENCE_ADAPTER_H */
