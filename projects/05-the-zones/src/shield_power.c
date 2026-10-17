/* projects/05-the-zones/src/shield_power.c: the fifteen lines the in-tree driver is missing.
 *
 * WHY THIS FILE EXISTS, AND WHY IT IS NOT THE PROJECT'S ANSWER. The part on this shield is held
 * unpowered until a supply-enable pin is driven high, and the operating system's driver for the
 * generation before it drives no pin at all: its binding is eleven lines and accepts no GPIO
 * property. So the run of Saturday 17 October 2026 at 11:16 failed at initialisation with the
 * vendor library's generic error, and it could not have succeeded at any address.
 *
 * THIS IS THE SMALLEST THING THAT SEPARATES THE TWO EXPLANATIONS. With the shield powered by
 * something outside the driver, the in-tree driver meets a part that is awake, and the next run
 * says which of these is true:
 *
 *   the device comes up    the driver for the previous generation drives this part once
 *                          something else powers the shield, and the gap in the tree is exactly
 *                          and only the power and low-power lines. That is a precise statement
 *                          and a short chapter
 *   it still fails         the two generations differ in something the driver can see, and the
 *                          chapter is the driver this project was going to write anyway, now
 *                          with the power question eliminated rather than suspected
 *
 * Either way the next run answers a question instead of adding a suspect, which the 11:16 run
 * did not. **The project's deliverable is still a binding that declares these pins**, because a
 * fact about the wiring belongs in the description of the hardware and not in an application
 * that happens to run first.
 *
 * THE PINS ARE NAMED BY ARDUINO POSITION AND NOT BY PORT, which is the one design decision in
 * the file. The vendor's own library for this shield names them `PWREN_PIN 11` and `LPN_PIN A3`,
 * which are positions on the connector rather than pins on a processor, and the mapping differs
 * between boards: on this one D11 is PA7 and A3 is PB1, while on the board the vendor tested, a
 * different Nucleo, A3 is a different port pin entirely. Writing the ports here would have been
 * right for this bench and wrong for every reader, so the overlay goes through the connector and
 * this file reads whatever the overlay resolved.
 */
#include <zephyr/devicetree.h>

/* COMPILED ONLY FOR THE DESCRIPTION THAT NEEDS IT. The overlay that points this part at the
 * tree's driver for the previous generation puts the control pins in `zephyr,user`, because that
 * driver's binding has no property for them. The overlay that points it at this project's own
 * driver puts them in the node, where the binding this project writes declares them, and the
 * driver drives them itself. So this file has nothing to do in the second case and compiles to
 * nothing, rather than being a second place the same pins are driven. */
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), pwren_gpios)

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shield_power, CONFIG_SENSOR_LOG_LEVEL);

#define USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec pwren = GPIO_DT_SPEC_GET(USER_NODE, pwren_gpios);
static const struct gpio_dt_spec lpn = GPIO_DT_SPEC_GET(USER_NODE, lpn_gpios);

/* THE TEN MILLISECONDS ARE THE VENDOR'S, not a guess at a settling time. ST's own example for
 * this shield drives the supply enable high and waits ten milliseconds before it constructs the
 * sensor object, so that is what is reproduced here rather than improved on. */
#define SETTLE_MS 10

/* RUNS BEFORE THE SENSOR DRIVER, and the ordering is asserted rather than assumed, because a
 * silent reordering would put this after the driver and the symptom would be identical to
 * having no file at all: the part unpowered when the driver speaks to it. */
#define SHIELD_POWER_INIT_PRIORITY 50
BUILD_ASSERT(SHIELD_POWER_INIT_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY,
	     "the shield has to be powered before the sensor driver initialises it");

static int shield_power_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&pwren) || !gpio_is_ready_dt(&lpn)) {
		LOG_ERR("the shield's control pins are not ready, so the part stays unpowered");
		return -ENODEV;
	}

	/* The supply first, then the part is brought out of its low-power state. Both are
	 * active high on this shield and the overlay says so, so this file asks for ACTIVE
	 * rather than for a level and cannot disagree with the description. */
	rc = gpio_pin_configure_dt(&pwren, GPIO_OUTPUT_ACTIVE);
	if (rc != 0) {
		LOG_ERR("could not drive the supply enable: %d", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&lpn, GPIO_OUTPUT_ACTIVE);
	if (rc != 0) {
		LOG_ERR("could not drive the low-power line: %d", rc);
		return rc;
	}

	k_msleep(SETTLE_MS);

	LOG_INF("shield powered: supply enable on %s pin %d, low power on %s pin %d, %d ms",
		pwren.port->name, pwren.pin, lpn.port->name, lpn.pin, SETTLE_MS);
	return 0;
}

SYS_INIT(shield_power_init, POST_KERNEL, SHIELD_POWER_INIT_PRIORITY);

#else

/* An empty translation unit is not C, so the file declares one unused type
 * rather than nothing at all. */
typedef int shield_power_not_used_t;

#endif
