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
#include "zones.h"
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

	/* The most recent result, and a flag saying whether one has ever arrived. Reading the
	 * grid before the first result would hand back a buffer of zeroes, which is the failure
	 * this project's first acceptance criterion exists to prevent. */
	VL53L8CX_ResultsData results;
	bool have_result;
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

/* THE SEQUENCE IS A PULSE AND NOT A LEVEL, which is the whole of what this project had wrong
 * from Saturday 17 October 2026 at 12:17 until 16:45. This driver drove both lines high and left
 * them there. ST's own application for this expansion board, under the comment "Sensor reset",
 * takes each line LOW, waits, then HIGH, waits:
 *
 *     PWR_EN low, 2 ms, PWR_EN high, 2 ms, LPn low, 2 ms, LPn high, 2 ms
 *
 * **A part that is already running does not notice the difference**, which is why every run on a
 * board that had been powered for hours worked, and why removing the pin driving altogether
 * changed nothing. A part that has just come up from cold is never taken out of reset, which is
 * why no run after the first power cycle found it on the bus at all.
 *
 * The two millisecond waits are theirs and are reproduced rather than improved on, as the ten
 * millisecond settle was. The difference is that this time the shape is theirs too.
 *
 * A line the binding does not carry is absent from the board, which is a description of hardware
 * rather than a failure, so this drives nothing and reports success. */
#define PULSE_MS 2

static int pulse_line(const struct gpio_dt_spec *line, const char *what)
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

	rc = gpio_pin_configure_dt(line, GPIO_OUTPUT_INACTIVE);
	if (rc != 0) {
		LOG_ERR("could not take the %s line low: %d", what, rc);
		return rc;
	}
	k_msleep(PULSE_MS);

	rc = gpio_pin_set_dt(line, 1);
	if (rc != 0) {
		LOG_ERR("could not take the %s line high: %d", what, rc);
		return rc;
	}
	k_msleep(PULSE_MS);

	/* THE PULSE REPORTS ITSELF, and the reason is that four runs were read as evidence about
	 * pin driving without anything ever confirming that a pin had been driven. A silent
	 * success is indistinguishable from a line the description does not carry, because the
	 * absent case returns success too. This line costs nothing and turns "we believe the
	 * pins were pulsed" into something a console says. */
	LOG_INF("%s pulsed low then high on %s pin %d, %d ms each", what, line->port->name,
		line->pin, PULSE_MS);
	return 0;
}

/* HOW LONG THE PART MAY TAKE TO ANSWER AFTER ITS SUPPLY IS ENABLED, and this bound exists
 * because of a control run on Saturday 17 October 2026 at 15:44. Every run before it had been a
 * processor reset on a board that was already powered, and the part answered immediately because
 * it had never stopped. The first run from a genuinely cold start, cable out and back in, found
 * **nothing at all on the bus**: not a wrong address, not a wrong jumper, nothing answering
 * anywhere.
 *
 * The vendor's own example waits ten milliseconds after driving the supply enable, and that is
 * what this driver did. On an Arduino that ten milliseconds follows a rail which has been stable
 * for as long as the sketch took to reach `setup()`. On a board coming up from cold it follows a
 * rail that is itself still settling, and ten milliseconds is not enough.
 *
 * SO THE DRIVER ASKS REPEATEDLY RATHER THAN WAITING A LONGER FIXED TIME, and reports how long it
 * took. A longer delay would be a number chosen to be safe; polling produces the figure instead,
 * and the figure is what the record should carry. */
#define IDENTITY_WAIT_MS 5000

/* FIVE SECONDS, RAISED FROM HALF A SECOND ON SATURDAY 17 OCTOBER 2026, and the raise is an
 * experiment rather than a fix. A cold start did not answer in 505 ms, which is far too long for
 * a settling rail, so the bound is opened wide enough to separate two possibilities that the
 * shorter one could not: **the part is slow, and we get a figure; or it is never going to
 * answer, and the fault is the pins or their order.** If the next cold start reports a number,
 * the bound comes back down to something near it with the figure beside it. If it reports five
 * seconds, this bound is not the thing to change. */

static int vl53l8cx_identify(const struct device *dev)
{
	const struct vl53l8cx_config *cfg = dev->config;
	struct vl53l8cx_data *data = dev->data;
	int64_t began = k_uptime_get();
	int64_t waited;
	int rc;

	do {
		rc = write_reg(&cfg->i2c, REG_PAGE, PAGE_IDENTITY);
		if (rc != 0) {
			k_msleep(5);
		}
	} while (rc != 0 && (k_uptime_get() - began) < IDENTITY_WAIT_MS);

	waited = k_uptime_get() - began;

	if (rc != 0) {
		LOG_ERR("no answer at 0x%02x after %d ms: the part is not powered, is held in "
			"its low-power state, or is not at this address",
			cfg->i2c.addr, (int)waited);
		return rc;
	}
	if (waited > 0) {
		/* THE FIGURE, PRINTED WHENEVER IT IS NOT ZERO, because it is the thing the
		 * control run was for and a driver that waited silently would hide it. */
		LOG_INF("the part answered %d ms after its supply was enabled", (int)waited);
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
	struct vl53l8cx_data *data = dev->data;
	uint8_t ready = 0u;

	ARG_UNUSED(chan);

	if (vl53l8cx_check_data_ready(&data->vendor, &ready) != 0u) {
		LOG_ERR("asking whether a result is ready failed");
		return -EIO;
	}
	if (ready == 0u) {
		/* NOT A FAULT. The part ranges at its own rate and a caller that asks more
		 * often than that gets this, which is how it paces itself without this driver
		 * inventing a delay for it. */
		return -EAGAIN;
	}

	if (vl53l8cx_get_ranging_data(&data->vendor, &data->results) != 0u) {
		LOG_ERR("a result was ready and did not come back");
		return -EIO;
	}

	data->have_result = true;
	return 0;
}

int zones_read(const struct device *dev, struct zones_grid *grid)
{
	struct vl53l8cx_data *data = dev->data;
	size_t i;

	if (grid == NULL) {
		return -EINVAL;
	}

	/* NOT A BUFFER OF ZEROES, which is this project's first acceptance criterion in one
	 * line: sixty-four zeroes look like an empty room and are more convincing than one zero
	 * for having structure. A caller that has not fetched gets told so. */
	if (!data->have_result) {
		return -EAGAIN;
	}

	for (i = 0u; i < (size_t)ZONES_TOTAL; i++) {
		grid->distance_mm[i] = data->results.distance_mm[i];
		grid->status[i] = data->results.target_status[i];
		grid->targets[i] = data->results.nb_target_detected[i];
	}
	grid->silicon_temp_degc = data->results.silicon_temp_degc;
	return 0;
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

	/* THE SUPPLY FIRST AND THE LOW-POWER LINE SECOND, in that order, because that is the
	 * order ST's own application uses and nothing here has a reason to prefer another. Each
	 * is pulsed rather than driven; see pulse_line for what that cost this project. */
	rc = pulse_line(&cfg->pwren, "supply enable");
	if (rc != 0) {
		return rc;
	}
	rc = pulse_line(&cfg->lpn, "low power");
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

	/* EIGHT BY EIGHT IS ASKED FOR AND NOT ASSUMED. The vendor's library starts at the
	 * smaller resolution, so a driver that never set this would quietly deliver sixteen
	 * zones to a project whose whole subject is sixty-four. */
	if (vl53l8cx_set_resolution(&data->vendor, VL53L8CX_RESOLUTION_8X8) != 0u) {
		LOG_ERR("the part would not take the eight by eight resolution");
		return -EIO;
	}

	/* THE RANGING RATE IS LEFT AT THE VENDOR'S DEFAULT, deliberately. The maximum differs
	 * by resolution and this project has not read this part's figure from a source, so
	 * setting one would be choosing a number from the previous generation's datasheet and
	 * hoping. The default is whatever the library ships, the run prints nothing about it,
	 * and the first capture that quotes a rate will have read it with `get_ranging_frequency_hz`
	 * rather than assumed it. */

	if (vl53l8cx_start_ranging(&data->vendor) != 0u) {
		LOG_ERR("the part would not start ranging");
		return -EIO;
	}

	LOG_INF("ranging started at eight by eight");
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
