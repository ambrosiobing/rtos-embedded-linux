/* projects/05-the-zones/src/identify.c: what the part says it is, before anything else asks it.
 *
 * THE QUESTION. The operating system's driver for the previous generation reaches this part,
 * uploads tens of kilobytes to it and then times out. The vendor library's own identification
 * check is four bus transactions and expects two specific bytes, so the cheapest thing a wired
 * board can tell us is whether this part answers them the way the previous generation does.
 *
 *   it answers 0xF0 and 0x02   the two generations are INDISTINGUISHABLE AT THE HANDSHAKE and
 *                              diverge only once the firmware image is booted. That is a precise
 *                              and publishable fact, and it also says the in-tree driver cannot
 *                              protect itself by checking: it will always think this part is one
 *                              of its own
 *   it answers anything else   the identification is where a driver should refuse, the in-tree
 *                              one did not, and the chapter gains a line about why
 *
 * Either answer is worth one build. Neither can be reasoned out.
 *
 * WHY THIS RUNS BEFORE THE DRIVER, at an initialisation priority between the shield's power and
 * the sensor driver's own. By the time an application's main() runs, the driver has already
 * poked this part, left it part way through an initialisation that failed, and selected a
 * register page of its own. A reading taken then would be of a part in an unknown state. Taking
 * it here means the part has been powered, has settled, and has been spoken to by nothing else.
 *
 * WHAT IT DOES TO THE PART. The vendor's own check writes a page selector, reads two bytes, and
 * writes the page selector back to the value it found useful. This file does exactly that and in
 * that order, from the library's source in the workspace rather than from any memory of a
 * register map, so that the part is left as the library's check leaves it and the driver that
 * follows meets what it would otherwise have met.
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(identify, CONFIG_SENSOR_LOG_LEVEL);

static const struct i2c_dt_spec part = I2C_DT_SPEC_GET(DT_ALIAS(zones));

/* From vl53l5cx_api.c, `vl53l5cx_is_alive`: a sixteen-bit register address, big endian on the
 * wire, the page selector at 0x7fff, the identity at 0 and 1, and the two values the previous
 * generation answers with. */
#define REG_PAGE     0x7fffu
#define REG_DEVICE   0x0000u
#define REG_REVISION 0x0001u
#define PAGE_IDENTITY 0x00u
#define PAGE_AFTER    0x02u
#define EXPECT_DEVICE   0xF0u
#define EXPECT_REVISION 0x02u

/* RUNS AFTER THE SHIELD IS POWERED AND BEFORE THE SENSOR DRIVER, and both halves of that are
 * asserted rather than assumed, because either one being wrong gives a reading of a part in the
 * wrong state and nothing in the output would say so. */
#define IDENTIFY_INIT_PRIORITY 60
BUILD_ASSERT(IDENTIFY_INIT_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY,
	     "the part has to be read before the sensor driver has spoken to it");

static int write_reg(uint16_t reg, uint8_t value)
{
	uint8_t buf[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xffu), value };

	return i2c_write_dt(&part, buf, sizeof(buf));
}

static int read_reg(uint16_t reg, uint8_t *value)
{
	uint8_t addr[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xffu) };

	return i2c_write_read_dt(&part, addr, sizeof(addr), value, 1);
}

static int identify_init(void)
{
	uint8_t device = 0u;
	uint8_t revision = 0u;
	int rc;

	if (!device_is_ready(part.bus)) {
		LOG_ERR("the bus is not ready, so the part was not asked what it is");
		return -ENODEV;
	}

	rc = write_reg(REG_PAGE, PAGE_IDENTITY);
	if (rc != 0) {
		/* NOT A GUESS AT WHY. A part that does not acknowledge here is a part that is not
		 * answering at all, which the bus scan in main() reports separately and better. */
		LOG_ERR("the part did not accept the page selector: %d", rc);
		return rc;
	}

	rc = read_reg(REG_DEVICE, &device);
	if (rc == 0) {
		rc = read_reg(REG_REVISION, &revision);
	}
	if (rc != 0) {
		LOG_ERR("the identity registers did not read back: %d", rc);
		return rc;
	}

	(void)write_reg(REG_PAGE, PAGE_AFTER);

	LOG_INF("the part identifies as device 0x%02x revision 0x%02x", device, revision);
	LOG_INF("the previous generation answers device 0x%02x revision 0x%02x",
		EXPECT_DEVICE, EXPECT_REVISION);

	if (device == EXPECT_DEVICE && revision == EXPECT_REVISION) {
		LOG_INF("SAME: the two generations cannot be told apart at the handshake, so the");
		LOG_INF("  in-tree driver cannot refuse this part and will try to drive it");
	} else {
		LOG_INF("DIFFERENT: the handshake tells the two apart, so a driver could refuse");
		LOG_INF("  this part here rather than after uploading an image to it");
	}

	return 0;
}

SYS_INIT(identify_init, POST_KERNEL, IDENTIFY_INIT_PRIORITY);
