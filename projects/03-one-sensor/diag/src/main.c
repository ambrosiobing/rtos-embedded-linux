/* projects/03-one-sensor/diag/src/main.c: which address answers, and what it says it is.
 *
 * THIS EXISTS BECAUSE A LINE OF OUTPUT WAS NOT ENOUGH. On Tuesday 6 October 2026 the
 * project's own application ran on the board and printed
 *
 *     FAIL adxl345@53 is not ready, so nothing below would be a reading
 *
 * which is its guard working, and which names none of the five things that could have caused
 * it: the part answering at 0x1d because SDO is not held low, the part in four-wire mode
 * because CS is not held high, the clock and data leads swapped, the bus clocked faster than
 * the pull-up resistors support, or the controller never coming up at all.
 *
 * None of those is distinguishable by looking at a breadboard, and there is no multimeter on
 * this bench. They are all distinguishable by printing which addresses answer and what the
 * device identification register holds, which is what this does.
 *
 * It reads the address and the bus rate OUT OF THE DEVICETREE rather than repeating them, so
 * the scan cannot disagree with the description it is testing. That matters here more than
 * usual: a diagnostic carrying its own copy of the address would pass while the overlay was
 * wrong, which is the one outcome that would waste the whole evening.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>   /* BIT and ARRAY_SIZE, named rather than inherited */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MOTION_NODE DT_ALIAS(motion)
#define BUS_NODE    DT_BUS(MOTION_NODE)

static const struct device *const bus = DEVICE_DT_GET(BUS_NODE);

/* From the description, not from this file. */
#define CONFIGURED_ADDR DT_REG_ADDR(MOTION_NODE)
#define CONFIGURED_HZ   DT_PROP(BUS_NODE, clock_frequency)

/* From the ADXL345 datasheet: register 0x00 is DEVID and reads 0xe5 on every part ever
 * made, which is what makes it the right thing to ask for. A part that answers its address
 * but returns something else is a different part, not a broken one. */
#define ADXL345_REG_DEVID 0x00U
#define ADXL345_DEVID     0xe5U

/* The registers that decide what a reading MEANS, as opposed to whether there is one.
 * DATA_FORMAT is the one the scale question turns on: bits 1 and 0 are the range, and bit 3
 * is full resolution, in which one count is 3.9 mg at EVERY range rather than following the
 * range ladder. The driver writes the range bits and never writes bit 3. */
#define ADXL345_REG_OFSX        0x1eU
#define ADXL345_REG_BW_RATE     0x2cU
#define ADXL345_REG_POWER_CTL   0x2dU
#define ADXL345_REG_DATA_FORMAT 0x31U
#define ADXL345_REG_DATAX0      0x32U

/* The two addresses the ADXL345 can have, and the pin that chooses between them. This is
 * the whole purpose of jumper A on the breadboard. */
#define ADDR_SDO_LOW  0x53U
#define ADDR_SDO_HIGH 0x1dU

#define SCAN_FIRST 0x08U
#define SCAN_LAST  0x77U

static bool report_devid(uint8_t addr, const char *claim)
{
	uint8_t value = 0U;
	int rc = i2c_reg_read_byte(bus, addr, ADXL345_REG_DEVID, &value);

	if (rc != 0) {
		printf("  0x%02x  %-34s silent, i2c_reg_read_byte returned %d\n",
		       addr, claim, rc);
		return false;
	}

	printf("  0x%02x  %-34s DEVID 0x%02x, %s\n", addr, claim, value,
	       value == ADXL345_DEVID ? "an ADXL345" : "not 0xe5, so not an ADXL345");

	return value == ADXL345_DEVID;
}

/* Integer only, because prj.conf leaves floating point formatting out and a magnitude is the
 * one place a square root cannot be avoided. Newton's method on integers, which terminates. */
static uint32_t isqrt(uint32_t n)
{
	uint32_t x = n;
	uint32_t y = (n + 1U) / 2U;

	if (n == 0U) {
		return 0U;
	}
	while (y < x) {
		x = y;
		y = (x + n / x) / 2U;
	}
	return x;
}

/* READ BACK WHAT RAN BEFORE THIS LEFT IN THE PART. This diagnostic configures nothing, which
 * is deliberate: the module keeps its supply across a processor reset, so the registers still
 * hold whatever the last application wrote. Run the project's application first and this
 * second, and the part reports what the driver programmed rather than what anyone assumed. */
static void dump_part(uint8_t addr)
{
	static const struct {
		uint8_t reg;
		const char *name;
	} regs[] = {
		{ ADXL345_REG_DEVID,       "DEVID" },
		{ ADXL345_REG_OFSX,        "OFSX" },
		{ ADXL345_REG_OFSX + 1U,   "OFSY" },
		{ ADXL345_REG_OFSX + 2U,   "OFSZ" },
		{ ADXL345_REG_BW_RATE,     "BW_RATE" },
		{ ADXL345_REG_POWER_CTL,   "POWER_CTL" },
		{ ADXL345_REG_DATA_FORMAT, "DATA_FORMAT" },
	};
	uint8_t raw[6];
	uint8_t fmt = 0U;
	unsigned int range, one_g, mag;
	int32_t ax, ay, az;
	size_t i;
	int rc;

	printf("\nthe part's registers, as whatever ran before this left them\n");
	for (i = 0U; i < ARRAY_SIZE(regs); i++) {
		uint8_t value = 0U;

		rc = i2c_reg_read_byte(bus, addr, regs[i].reg, &value);
		if (rc != 0) {
			printf("  0x%02x  %-12s read failed, %d\n",
			       regs[i].reg, regs[i].name, rc);
			return;
		}
		printf("  0x%02x  %-12s 0x%02x\n", regs[i].reg, regs[i].name, value);
		if (regs[i].reg == ADXL345_REG_DATA_FORMAT) {
			fmt = value;
		}
	}

	range = fmt & 0x03U;
	printf("\nDATA_FORMAT 0x%02x, bit by bit\n", fmt);
	printf("  range     %u, which is plus and minus %u g\n", range, 2U << range);
	printf("  FULL_RES  %s\n", (fmt & BIT(3)) ? "SET" : "clear");
	printf("  JUSTIFY   %s\n", (fmt & BIT(2)) ? "SET" : "clear");
	printf("  SELF_TEST %s\n", (fmt & BIT(7)) ? "SET" : "clear");
	printf("  SPI_3WIRE %s\n", (fmt & BIT(6)) ? "SET" : "clear");

	rc = i2c_burst_read(bus, addr, ADXL345_REG_DATAX0, raw, sizeof(raw));
	if (rc != 0) {
		printf("\n  the six data registers would not read, %d\n", rc);
		return;
	}

	ax = (int16_t)((uint16_t)raw[1] << 8 | raw[0]);
	ay = (int16_t)((uint16_t)raw[3] << 8 | raw[2]);
	az = (int16_t)((uint16_t)raw[5] << 8 | raw[4]);

	/* One g in counts. In full resolution it is 256 at every range, because the count size
	 * is fixed and the data widens instead; in ten bit mode it halves each time the range
	 * doubles. THIS IS THE WHOLE QUESTION, and it is now read rather than assumed. */
	one_g = (fmt & BIT(3)) ? 256U : (256U >> range);
	mag = isqrt((uint32_t)(ax * ax + ay * ay + az * az));

	printf("\nraw counts   x %6d   y %6d   z %6d\n", ax, ay, az);
	printf("vector       %u counts\n", mag);
	printf("one g is     %u counts in this mode\n", one_g);
	printf("so the part reads %u milli g, where a part sitting still must read 1000\n",
	       (unsigned int)(((uint32_t)mag * 1000U) / one_g));
}

int main(void)
{
	unsigned int addr;
	int found = 0;

	printf("P03 bus scan, written because one FAIL line named nothing\n\n");

	printf("controller        %s\n", bus->name);
	printf("clock             %u Hz, from the description\n", (unsigned int)CONFIGURED_HZ);
	printf("expected address  0x%02x, from the description\n\n",
	       (unsigned int)CONFIGURED_ADDR);

	/* Checked before anything is concluded from silence. A controller that never
	 * initialised produces exactly the same empty scan as a part that is not wired, and
	 * only one of those is about the breadboard. */
	if (!device_is_ready(bus)) {
		printf("FAIL the controller itself is not ready, so no address could have\n");
		printf("     answered and nothing below would have meant anything\n");
		return 1;
	}

	printf("scanning 0x%02x to 0x%02x\n", SCAN_FIRST, SCAN_LAST);
	for (addr = SCAN_FIRST; addr <= SCAN_LAST; addr++) {
		uint8_t byte;

		/* One byte read rather than a zero-length write. A zero-length transfer is
		 * the conventional probe and some controllers refuse it outright, which
		 * would report every address as absent and look like a dead bus. */
		if (i2c_read(bus, &byte, 1, (uint16_t)addr) == 0) {
			printf("  answer at 0x%02x\n", addr);
			found++;
		}
	}
	printf("%d address(es) answered\n\n", found);

	printf("the two addresses this part can have, and what each would mean\n");
	if (report_devid(ADDR_SDO_LOW, "SDO low, so jumper A is working")) {
		dump_part(ADDR_SDO_LOW);
	}
	report_devid(ADDR_SDO_HIGH, "SDO high, so jumper A is not");

	printf("\n");
	if (found == 0) {
		printf("NOTHING ANSWERED. The next question is the clock, not the leads:\n");
		printf("  rebuild with overlays/slow.overlay and compare.\n");
	}

	return 0;
}
