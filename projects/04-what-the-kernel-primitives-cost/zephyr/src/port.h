/* projects/04-what-the-kernel-primitives-cost/zephyr/src/port.h
 *
 * ONE DECLARATION, AND THE NAME IS THE POINT. The invalidation for criterion 3 lived as a
 * static `chill()` in main.c while only main.c needed it. On Saturday 17 October 2026 the
 * contended case joined the cold pass, and mutex_case.c needs the same call.
 *
 * It is named `measure_port_chill` because that is what the FreeRTOS tree's port.h calls it,
 * so the line inside each kernel's contended case is character for character the same. Two
 * implementations of one measurement are two chances to measure two different things, and
 * nothing inside either notices; a line that is textually identical in both can at least be
 * compared by eye and by grep. The FreeRTOS tree splits its port into a host file and a board
 * file and needs a header to join them; this tree has one port, in main.c, and needs the
 * header only for this.
 */
#ifndef P04_ZEPHYR_PORT_H
#define P04_ZEPHYR_PORT_H

/* Invalidates the instruction cache when the cold pass has asked for it, and does nothing
 * otherwise. Called at the statement before a bracket opens, which is in the measurer for most
 * rows and in the giver for the two that block, because where a bracket opens differs by
 * operation. */
void measure_port_chill(void);

#endif /* P04_ZEPHYR_PORT_H */
