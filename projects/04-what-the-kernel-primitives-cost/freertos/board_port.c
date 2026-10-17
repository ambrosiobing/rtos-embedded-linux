/* projects/04-what-the-kernel-primitives-cost/freertos/board_port.c: the port, on the board.
 *
 * TWO COUNTERS, CHOSEN THE SAME WAY THE ZEPHYR PORT CHOOSES THEM, because the choice is a
 * property of the part and not of the kernel. The core instrument is the data watchpoint
 * unit's cycle counter, which Zephyr's timing API and its own benchmark suite read, and which
 * stops when the core stops. The wall instrument is derived from the system tick, which keeps
 * running through idle, and it is the only one of the two that can bracket a sleep. Established
 * on Friday 9 October 2026 at 09:20, by a gate that refused: 4260 counts for a 200 ms sleep.
 *
 * Both run at the core clock, 280 MHz, and the port reports that rate for both rather than
 * assuming it: board_core_hz() is decoded from the clock registers after the climb, and if the
 * climb did not reach 280 the main refuses to start the scheduler at all.
 */
#include "port.h"

#include "../adapter/measure.h"

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "stm32h7a3_regs.h"

/* ---- the core instrument: the cycle counter ---------------------------------------------- */

uint32_t measure_now(void)
{
	return board_cycles_now();
}

uint32_t measure_clock_hz(void)
{
	/* Zero if the counter never advanced in board_cycles_init()'s own check, which makes
	 * every capture refused rather than filled with a stale register value. */
	return board_cycles_available() ? board_core_hz() : 0u;
}

/* ---- the wall instrument: ticks and the SysTick count between them ------------------------ */

/* THE SAME CONSTRUCTION ZEPHYR'S SYSTICK DRIVER USES for k_cycle_get_32(): the tick count
 * times the reload, plus how far the current period has run. FreeRTOS owns SysTick as its
 * tick source and clocks it from the core, so the reload is the core rate over the tick rate
 * and the resolution is one core cycle.
 *
 * THE RACE, AND WHY THE FLAG IS READ. SysTick wraps, then the tick interrupt increments the
 * count; a read between the two sees the old count and a current value near the reload, which
 * is one whole period short. COUNTFLAG is set by the wrap and cleared by reading the control
 * register, and the kernel's handler never reads it, so inside a critical section it says
 * exactly whether a wrap is pending. If it is, the current value is read again, after the
 * wrap, and the count is carried by one. */
uint32_t measure_now_wall(void)
{
	uint32_t ticks, cvr, reload;

	taskENTER_CRITICAL();
	reload = SYST_RVR & SYST_RELOAD_MAX;
	ticks = (uint32_t)xTaskGetTickCount();
	cvr = SYST_CVR & SYST_RELOAD_MAX;
	if ((SYST_CSR & (1u << SYST_CSR_COUNTFLAG_POS)) != 0u) {
		cvr = SYST_CVR & SYST_RELOAD_MAX;
		ticks++;
	}
	taskEXIT_CRITICAL();

	return ticks * (reload + 1u) + (reload - cvr);
}

uint32_t measure_clock_hz_wall(void)
{
	/* SysTick on the core clock, which is what the port selects when
	 * configSYSTICK_CLOCK_HZ is left undefined, so the wall rate is the core rate. */
	return board_core_hz();
}

/* ---- criterion 3: the instruction cache -------------------------------------------------- */

/* The same three steps the Zephyr port's chill() performs through CMSIS, on the registers the
 * sibling volume's icache.c already names: invalidate all, then the two barriers. The
 * instruction cache only, for the reason recorded in the Zephyr adapter: invalidating the
 * data cache without cleaning it first would discard anything dirty. */
static volatile bool cold_mode;

void measure_port_set_cold(bool cold)
{
	cold_mode = cold;
}

void measure_port_chill(void)
{
	if (cold_mode) {
		SCB_ICIALLU = 0u;
		__asm volatile ("dsb");
		__asm volatile ("isb");
	}
}

/* ---- the marker: PB4, as docs/WIRING.md records and the Zephyr overlay names -------------- */

/* PB4 is the same pin the Zephyr build drives, gpio@58020400 pin 4 in its devicetree, so the
 * lead already fitted to CN7 pin 19 and the MCC 118's CH0 needs no change for this kernel.
 * Port B's clock is enabled by board_init() for the LEDs and is enabled again here, because
 * this file must not depend on that, and a write to a port whose clock is off is discarded in
 * silence. The read-back after the enable is the same bus-bridge precaution board.c takes. */
#define MARKER_PORT GPIOB
#define MARKER_PIN  4u

static bool marker_ready;

void measure_port_marker_init(void)
{
	uint32_t moder;

	RCC_AHB4ENR |= (1u << RCC_AHB4ENR_GPIOBEN_POS);
	(void)RCC_AHB4ENR;

	/* Output, push-pull, no pull, low speed: the two-bit mode field for the pin set to 01
	 * and everything else left at its reset value of zero. Driven low to start, so the
	 * first toggle is the first rising edge. */
	moder = GPIO_REG(MARKER_PORT, GPIO_MODER);
	moder &= ~(3u << (MARKER_PIN * 2u));
	moder |= (1u << (MARKER_PIN * 2u));
	GPIO_REG(MARKER_PORT, GPIO_MODER) = moder;
	GPIO_REG(MARKER_PORT, GPIO_BSRR) = (1u << (MARKER_PIN + 16u));

	marker_ready = (GPIO_REG(MARKER_PORT, GPIO_MODER) & (3u << (MARKER_PIN * 2u))) ==
		       (1u << (MARKER_PIN * 2u));
}

bool measure_port_marker_ready(void)
{
	return marker_ready;
}

void measure_port_marker_toggle(void)
{
	/* One writer, so an exclusive-or on the output register is safe, and it is what the
	 * sibling volume's board_led_toggle does. */
	GPIO_REG(MARKER_PORT, GPIO_ODR) ^= (1u << MARKER_PIN);
}
