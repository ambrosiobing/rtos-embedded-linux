/* projects/04-what-the-kernel-primitives-cost/freertos/host_port.c: the port, on a host.
 *
 * THE PORT REPORTS NO CONFIRMED CLOCK, AND THAT IS THE WHOLE POINT OF A HOST BUILD. The POSIX
 * port has no cycle counter. measure_now() reads the monotonic clock in units of a hundred
 * nanoseconds so that brackets come back nonzero and in order, and both rates return zero, so
 * every capture this build emits carries `clock_hz 0` and the reduction refuses it. A number
 * that cannot be a measurement should not be publishable by accident, and this build cannot
 * publish one.
 */
#include "port.h"

#include "../adapter/measure.h"

#include <time.h>

/* A HUNDRED NANOSECONDS PER COUNT, and the unit is chosen for the wrap guard rather than for
 * resolution. Nanoseconds would wrap a 32-bit count every 4.3 seconds, inside one run of this
 * application; a hundred nanoseconds wraps every 429 seconds, outside it. Microseconds would
 * risk a zero count on the fastest bracket, which the core would then refuse for the wrong
 * reason. None of this makes the counts times: the rate below is zero. */
static uint32_t monotonic_100ns(void)
{
	struct timespec ts;

	(void)clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)((uint64_t)ts.tv_sec * 10000000u + (uint64_t)ts.tv_nsec / 100u);
}

uint32_t measure_now(void)
{
	return monotonic_100ns();
}

uint32_t measure_now_wall(void)
{
	return monotonic_100ns();
}

/* ZERO, ON PURPOSE, FOR BOTH. A host clock is confirmed against nothing this project trusts,
 * so the capture says so and the reduction refuses it. Returning the true rate of the monotonic
 * clock would be easy and would produce a table of host figures that nobody should publish. */
uint32_t measure_clock_hz(void)
{
	return 0u;
}

uint32_t measure_clock_hz_wall(void)
{
	return 0u;
}

/* No cache, so nothing to make cold. The campaign is handed a NULL set_cold hook and says
 * criterion 3 does not exist here; these two exist only so the adapter links. */
void measure_port_set_cold(bool cold)
{
	(void)cold;
}

void measure_port_chill(void)
{
}

/* No pin, and the period row still runs: on a host every capture is refused for its clock
 * before anything else about it is looked at, and what the run shows is that the row
 * completes. Reporting ready is what lets it. */
void measure_port_marker_init(void)
{
}

bool measure_port_marker_ready(void)
{
	return true;
}

void measure_port_marker_toggle(void)
{
}
