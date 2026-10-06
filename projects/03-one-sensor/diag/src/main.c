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

/* The two addresses the ADXL345 can have, and the pin that chooses between them. This is
 * the whole purpose of jumper A on the breadboard. */
#define ADDR_SDO_LOW  0x53U
#define ADDR_SDO_HIGH 0x1dU

#define SCAN_FIRST 0x08U
#define SCAN_LAST  0x77U

static void report_devid(uint8_t addr, const char *claim)
{
	uint8_t value = 0U;
	int rc = i2c_reg_read_byte(bus, addr, ADXL345_REG_DEVID, &value);

	if (rc != 0) {
		printf("  0x%02x  %-34s silent, i2c_reg_read_byte returned %d\n",
		       addr, claim, rc);
		return;
	}

	printf("  0x%02x  %-34s DEVID 0x%02x, %s\n", addr, claim, value,
	       value == ADXL345_DEVID ? "an ADXL345" : "not 0xe5, so not an ADXL345");
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
	report_devid(ADDR_SDO_LOW, "SDO low, so jumper A is working");
	report_devid(ADDR_SDO_HIGH, "SDO high, so jumper A is not");

	printf("\n");
	if (found == 0) {
		printf("NOTHING ANSWERED. The next question is the clock, not the leads:\n");
		printf("  rebuild with overlays/slow.overlay and compare.\n");
	}

	return 0;
}
