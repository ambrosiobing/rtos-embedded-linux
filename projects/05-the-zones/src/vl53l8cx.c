/* projects/05-the-zones/src/vl53l8cx.c: a driver for this generation, and it does not range yet.
 *
 * WHAT IT DOES. It drives the supply enable and the low-power line from its own binding, waits,
 * asks the part what it is, refuses it by name or goes on, and sends it this generation's
 * firmware image. On Saturday 17 October 2026 at 15:20 the part accepted that image in 1.971
 * seconds, where the previous generation's image left the tree's driver timing out after 5.924.
 * It does not read the grid; that is the next piece of work and everything above it is done.
 *
 * It exists in this form because the afternoon of Saturday 17 October 2026 established exactly
 * these things on the bench and nothing more, and a driver that claimed more would be claiming
 * what no run here supports.
 *
 * THE REFUSAL IS THE POINT. The operating system's driver for the previous generation reaches a
 * part of this family, does not ask what it is, uploads tens of kilobytes of firmware image to
 * it and reports a timeout about six seconds later. The vendor's own library contains the check
 * it needs, four bus transactions, and that driver does not call it. On this bench that is the
 * difference between a console that says which part is fitted and one that says `(1)`.
 *
 * SO THIS DRIVER ASKS FIRST, and the three cases it distinguishes are chosen by what can be
 * justified rather than by what would be tidy:
 *
 *   the family byte is wrong      refused. Whatever is at this address, it is not this family,
 *                                 and no firmware image should be sent to it
 *   the revision is the previous  refused BY NAME. The tree already has a driver for that part
 *     generation's               and this is not it, so the message says which driver to use
 *                                 rather than that something went wrong
 *   anything else                 accepted, with the revision logged
 *
 * **THE LAST CASE IS DELIBERATE AND THE REASON IS ONE OBSERVATION.** This bench has seen one
 * part of this generation and it answers revision 0x0C. One example is not a specification, so
 * this driver does not refuse an unknown revision: it would be refusing on the strength of a
 * sample of one, and a reader with a later part would be told their hardware is wrong by a file
 * that had never seen it. It logs what it found instead, which is the honest form of the same
 * information.
 */
#define DT_DRV_COMPAT st_vl53l8cx

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vl53l8cx, CONFIG_SENSOR_LOG_LEVEL);

/* The vendor's interface for this generation, cloned at a pin and compiled from where it lies;
 * see CMakeLists.txt for why nothing is copied in. `platform.h` is this project's and is on the
 * include path first, so the library finds the six functions in src/vl53l8cx_platform.c. */
#include "platform.h"
#include <vl53l8cx_api.h>

/* From `vl53l5cx_is_alive` in the vendor library in the workspace: a sixteen-bit register
 * address, big endian on the wire, a page selector, and the identity at the first two
 * registers of page zero. The same four transactions serve both generations, which is itself
 * the finding: they agree on how to be asked and disagree on the answer. */
#define REG_PAGE       0x7fffu
#define REG_DEVICE     0x0000u
#define REG_REVISION   0x0001u
#define PAGE_IDENTITY  0x00u
#define PAGE_AFTER     0x02u

#define DEVICE_ID_FAMILY     0xF0u  /* answered by both generations */
#define REVISION_PREVIOUS    0x02u  /* the VL53L5CX, which the tree already drives */
#define REVISION_SEEN_HERE   0x0Cu  /* this bench's part, on Saturday 17 October 2026, n = 1 */

/* The vendor's own example for this expansion board waits this long after the supply enable
 * before it speaks to the part. A shorter wait was not tried, so this is reproduced rather
 * than improved on. */
#define SETTLE_MS 10

struct vl53l8cx_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec pwren;
	struct gpio_dt_spec lpn;
	struct gpio_dt_spec irq;
};

struct vl53l8cx_data {
	uint8_t device_id;
	uint8_t revision_id;

	/* The vendor's own state, which includes the buffer its firmware image is carried in.
	 * It is large and it is per instance, which is the vendor's design and not a choice
	 * this driver gets to make. */
	VL53L8CX_Configuration vendor;
};

static int write_reg(const struct i2c_dt_spec *i2c, uint16_t reg, uint8_t value)
{
	uint8_t buf[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xffu), value };

	return i2c_write_dt(i2c, buf, sizeof(buf));
}

static int read_reg(const struct i2c_dt_spec *i2c, uint16_t reg, uint8_t *value)
{
	uint8_t addr[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xffu) };

	return i2c_write_read_dt(i2c, addr, sizeof(addr), value, 1);
}

/* A line the binding does not have to carry. Absent means the board does not have it, which is
 * a description of hardware rather than a failure, so this returns success and drives nothing. */
static int assert_line(const struct gpio_dt_spec *line, const char *what)
{
	int rc;

	if (line->port == NULL) {
		return 0;
	}
	if (!gpio_is_ready_dt(line)) {
		LOG_ERR("the %s line is named in the description and its controller is not ready",
			what);
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(line, GPIO_OUTPUT_ACTIVE);
	if (rc != 0) {
		LOG_ERR("could not drive the %s line: %d", what, rc);
	}
	return rc;
}

static int vl53l8cx_identify(const struct device *dev)
{
	const struct vl53l8cx_config *cfg = dev->config;
	struct vl53l8cx_data *data = dev->data;
	int rc;

	rc = write_reg(&cfg->i2c, REG_PAGE, PAGE_IDENTITY);
	if (rc != 0) {
		LOG_ERR("no answer at 0x%02x: the part is not powered, is held in its low-power "
			"state, or is not at this address", cfg->i2c.addr);
		return rc;
	}

	rc = read_reg(&cfg->i2c, REG_DEVICE, &data->device_id);
	if (rc == 0) {
		rc = read_reg(&cfg->i2c, REG_REVISION, &data->revision_id);
	}
	if (rc != 0) {
		LOG_ERR("the identity registers did not read back: %d", rc);
		return rc;
	}

	(void)write_reg(&cfg->i2c, REG_PAGE, PAGE_AFTER);
	return 0;
}

static int vl53l8cx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	/* NOT A STUB THAT RETURNS ZEROES, which is this project's first acceptance criterion in
	 * one line: a grid of zeroes looks like an empty room and is more convincing than a
	 * single zero for having structure. Ranging needs this generation's vendor interface and
	 * this driver does not have it yet, so it says so in the only way an interface allows. */
	return -ENOTSUP;
}

static int vl53l8cx_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(val);

	return -ENOTSUP;
}

static DEVICE_API(sensor, vl53l8cx_driver_api) = {
	.sample_fetch = vl53l8cx_sample_fetch,
	.channel_get = vl53l8cx_channel_get,
};

/* NAMED `_dev_init` AND NOT `_init`, because the vendor's library already has a function called
 * `vl53l8cx_init` and this one would shadow it; the compiler said so plainly the first time the
 * two met. The operating system's driver for the previous generation calls its own
 * `vl53l5cx_dev_init` for the same reason, which is a convention worth following rather than
 * an accident worth repeating. */
static int vl53l8cx_dev_init(const struct device *dev)
{
	const struct vl53l8cx_config *cfg = dev->config;
	struct vl53l8cx_data *data = dev->data;
	int rc;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("the bus is not ready");
		return -ENODEV;
	}

	rc = assert_line(&cfg->pwren, "supply enable");
	if (rc != 0) {
		return rc;
	}
	rc = assert_line(&cfg->lpn, "low power");
	if (rc != 0) {
		return rc;
	}
	if (cfg->pwren.port != NULL || cfg->lpn.port != NULL) {
		k_msleep(SETTLE_MS);
	}

	rc = vl53l8cx_identify(dev);
	if (rc != 0) {
		return -EIO;
	}

	if (data->device_id != DEVICE_ID_FAMILY) {
		LOG_ERR("device 0x%02x at 0x%02x is not this family, which answers 0x%02x. "
			"Nothing is sent to it", data->device_id, cfg->i2c.addr,
			DEVICE_ID_FAMILY);
		return -ENODEV;
	}

	if (data->revision_id == REVISION_PREVIOUS) {
		LOG_ERR("revision 0x%02x is the previous generation, which this driver is not for. "
			"Describe it as st,vl53l5cx, which the tree already carries",
			data->revision_id);
		return -ENODEV;
	}

	LOG_INF("device 0x%02x revision 0x%02x at 0x%02x", data->device_id, data->revision_id,
		cfg->i2c.addr);
	if (data->revision_id != REVISION_SEEN_HERE) {
		LOG_WRN("revision 0x%02x has not been seen by this project, which has met 0x%02x "
			"on one part. It is accepted rather than refused, because one example is "
			"not a specification", data->revision_id, REVISION_SEEN_HERE);
	}
	/* THE VENDOR'S INITIALISATION, WHICH IS WHERE THE FIRMWARE IMAGE GOES. Everything above
	 * this line is four bus transactions and costs a millisecond; this is tens of kilobytes
	 * and takes seconds. Doing the cheap refusal first is the whole argument of this driver
	 * against the one in the tree, so the order is load bearing and not a style. */
	data->vendor.platform.i2c = cfg->i2c;

	rc = (int)vl53l8cx_init(&data->vendor);
	if (rc != 0) {
		LOG_ERR("the vendor's initialisation refused this part: %d. The identity above "
			"was accepted, so this is not a wrong part and not a wrong address", rc);
		return -EIO;
	}

	LOG_INF("initialised with this generation's own interface");
	return 0;
}

#define VL53L8CX_INIT(i)                                                                       \
	static struct vl53l8cx_data vl53l8cx_data_##i;                                         \
	static const struct vl53l8cx_config vl53l8cx_config_##i = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(i),                                                \
		.pwren = GPIO_DT_SPEC_INST_GET_OR(i, pwren_gpios, { 0 }),                      \
		.lpn = GPIO_DT_SPEC_INST_GET_OR(i, lpn_gpios, { 0 }),                          \
		.irq = GPIO_DT_SPEC_INST_GET_OR(i, int_gpios, { 0 }),                          \
	};                                                                                     \
	SENSOR_DEVICE_DT_INST_DEFINE(i, vl53l8cx_dev_init, NULL, &vl53l8cx_data_##i,               \
				     &vl53l8cx_config_##i, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &vl53l8cx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VL53L8CX_INIT)
