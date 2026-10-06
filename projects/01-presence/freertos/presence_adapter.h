/* projects/01-presence/freertos/presence_adapter.h: the FreeRTOS adapter.
 *
 * WHAT AN ADAPTER IS. It carries events and decides nothing. docs/RTOS_VARIANTS.md
 * states that as a negative, which is the useful form: there is no comparison
 * against a range in this file or in presence_adapter.c, no count of readings, no
 * notion of how long a hold lasts, and no opinion about what an event means. Every
 * number lives in presence_settings_t and every decision is a row of the table.
 *
 * THE THREE THINGS THE TABLE NEEDS BACK, which this adapter has to provide:
 *
 *   1. One queue, one consumer, first in first out. xQueueSendToBack and one
 *      dispatch task. xQueueSendToFront and xQueueOverwrite are never called, and
 *      the reason is that either would silently remove this guarantee: row 17 is
 *      guarded, but rows 4, 11 and 18 all exist because the ORDER of events is what
 *      makes a stale expiry harmless.
 *   2. at_ms stamped when an event is posted, never when it is dispatched. Every
 *      post in this adapter reads the tick count at the moment of posting, and the
 *      timer callbacks read it when they fire. An adapter that stamped at the front
 *      of the dispatch loop would make every stale event look fresh and defeat the
 *      guard on row 17.
 *   3. A cancelled hold may still deliver its expiry. xTimerStop posts a command to
 *      the timer service task rather than acting at once, so the window here is
 *      wider than Zephyr's. The table absorbs it in rows 4, 11 and 18 rather than
 *      the adapter dropping anything.
 *
 * THE ONE PIECE OF STATE THIS ADAPTER READS is hold_running, and only to mirror it
 * onto the kernel timer: false to true arms, true to false cancels. It reads it
 * after presence_dispatch returns and never writes it.
 */
#ifndef PRESENCE_ADAPTER_H
#define PRESENCE_ADAPTER_H

#include "presence.h"

#include <stdbool.h>
#include <stdint.h>

/* Create the queue, the two timers and the dispatch task, all statically, and start
 * nothing. Call before the scheduler starts. Returns false if any object could not
 * be created, which with static allocation means a programming error rather than
 * exhaustion. */
bool presence_adapter_init(void);

/* Start the heartbeat, which posts PRESENCE_EV_TICK every period_ms for ever.
 *
 * Separate from init on purpose, and the reason is the host test rather than the
 * board. A periodic event arriving in the middle of a scripted sequence would
 * interleave TICK rows with the rows under test, and the sequence this directory
 * exists to compare against the direct-dispatch test would stop being comparable.
 * So the test runs its sequences with the heartbeat stopped and then starts it on
 * its own, which is also the only way to show that it works. The board's main calls
 * both, in order. */
bool presence_adapter_start_tick(uint32_t period_ms);

/* Post an event from a task. Returns false when the queue is full, which is an
 * event lost, which is how a release goes missing: the count is kept and
 * presence_adapter_events_dropped reports it. Nothing here ever blocks. */
bool presence_adapter_post(const presence_event_t *ev);

/* The same from an interrupt. Sets *higher_priority_task_woken for the caller to
 * yield on, as FreeRTOS requires. On a host build nothing calls this; it exists
 * because on the board the button and the sensor both arrive this way. */
bool presence_adapter_post_from_isr(const presence_event_t *ev,
                                    long *higher_priority_task_woken);

/* How many posts were refused because the queue was full. A dropped event cannot
 * be recovered, so the only honest thing is to count it and report it. Zero is the
 * only acceptable value in a run that claims to have measured anything. */
uint32_t presence_adapter_events_dropped(void);

/* Read-only access to the machine, for a shell command or a test. const, because
 * nothing outside the dispatch task may touch it. */
const presence_t *presence_adapter_context(void);

/* Called after every dispatch, with the row taken and the context after it. This is
 * observability and not a decision: the adapter does not look at what the hook
 * does, and the hook may not post events. The host test uses it to compare the row
 * sequence through the real queue against the sequence the direct-dispatch test
 * produces, which is the whole claim of this directory. */
typedef void (*presence_trace_fn)(uint8_t row, const presence_t *after);
void presence_adapter_set_trace(presence_trace_fn fn);

/* The three indicators, as a function of state. Defined by the application, once
 * per board, and deliberately not here: a lamp is the one thing in this project
 * that differs between a Nucleo and a host, and an adapter that knew about a GPIO
 * would be a third thing to port. The host build prints a line instead. */
void presence_indicators(presence_state_t state);

#endif /* PRESENCE_ADAPTER_H */
