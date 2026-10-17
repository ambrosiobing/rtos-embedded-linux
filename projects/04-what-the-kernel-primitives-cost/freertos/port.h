/* projects/04-what-the-kernel-primitives-cost/freertos/port.h: what the adapter asks of a port
 * beyond the contract.
 *
 * ../adapter/measure.h is the contract every kernel meets: two counters, two rates. The four
 * hooks here are things the FreeRTOS adapter needs that are not measurements and that differ
 * between a host and a board: a cache to make cold, and a pin to toggle. On a host every one
 * of them is a no-op that says so, and the adapter does not know which it is talking to.
 */
#ifndef P04_FREERTOS_PORT_H
#define P04_FREERTOS_PORT_H

#include <stdbool.h>

/* Criterion 3. When cold is set, every call to measure_port_chill() invalidates the
 * instruction cache; when clear, it does nothing. The adapter calls it at exactly the places
 * the Zephyr adapter does: the measurer's statement before its own stamp, and the giver's
 * statement before its stamp, because where a bracket opens differs by operation. */
void measure_port_set_cold(bool cold);
void measure_port_chill(void);

/* The marker the witness watches. Initialised once before the scheduler starts; ready says
 * whether there is a pin at all, so the period row can refuse rather than produce a capture
 * nothing could have seen. */
void measure_port_marker_init(void);
bool measure_port_marker_ready(void);
void measure_port_marker_toggle(void);

#endif /* P04_FREERTOS_PORT_H */
