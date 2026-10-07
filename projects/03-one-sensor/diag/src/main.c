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

/* HOW MANY ATTEMPTS A READ IS GIVEN, and why the count is printed rather than the outcome.
 *
 * On Wednesday 7 October 2026 the same register, at the same address, in the same run, read
 * successfully four times out of four in one place and failed four times out of four a few
 * milliseconds later in another. Contact noise does not usually sort itself that neatly, so
 * either one of these call sites is wrong or the probability of success depends on something
 * neither of them is reporting.
 *
 * A single attempt can only say "worked" or "did not", which is what produced that puzzle.
 * Counting the attempts turns the same run into a measurement: one try means a healthy
 * contact, three means a marginal one, and five failures in a row at a site that the previous
 * line just succeeded at is a software question rather than a wiring one. */
#define READ_TRIES 5

static int read_reg(uint8_t addr, uint8_t reg, uint8_t *value, int *tries)
{
	int rc = -1;
	int n;

	for (n = 1; n <= READ_TRIES; n++) {
		rc = i2c_reg_read_byte(bus, addr, reg, value);
		if (rc == 0) {
			break;
		}
	}

	*tries = (n > READ_TRIES) ? READ_TRIES : n;
	return rc;
}

static bool report_devid(uint8_t addr, const char *claim)
{
	uint8_t value = 0U;
	int tries = 0;
	int rc = read_reg(addr, ADXL345_REG_DEVID, &value, &tries);

	if (rc != 0) {
		printf("  0x%02x  %-34s silent after %d tries, last rc %d\n",
		       addr, claim, tries, rc);
		return false;
	}

	printf("  0x%02x  %-34s DEVID 0x%02x on try %d, %s\n", addr, claim, value, tries,
	       value == ADXL345_DEVID ? "an ADXL345" : "not 0xe5, so not an ADXL345");

	return value == ADXL345_DEVID;
}

/* CRITERION 6: two devices on one bus do not interfere.
 *
 * It lives here rather than in the project's application, and that is not an accident. The
 * application is not permitted to know there is a bus, so it cannot be the thing that proves
 * two devices share one. This diagnostic may, and does.
 *
 * It needs NO datasheet fact about the neighbour. An address acknowledgement is enough, so
 * nothing is asserted about a register map that has not been read: the neighbour is whichever
 * address answered the scan first that is not one of this part's two, and the operation is the
 * same zero-length probe the scan already uses.
 *
 * WHAT THE CRITERION ACTUALLY ASKS, which is easy to get wrong. It does not ask that the error
 * count be zero. A part held in unsoldered holes by friction fails sometimes on its own, and a
 * run demanding zero would fail for a reason that has nothing to do with sharing a bus. It
 * asks that the count be **no worse when the two are interleaved than when each runs alone**.
 * So each phase is run separately first, and the baseline is the thing the result is compared
 * against rather than an ideal.
 */
#define ROUNDS 200

static int probe_addr(uint8_t addr)
{
	uint8_t byte = 0U;

	return i2c_write(bus, &byte, 0, (uint16_t)addr);
}

static void interference(uint8_t part, uint8_t neighbour)
{
	unsigned int part_alone = 0U, nb_alone = 0U;
	unsigned int part_mixed = 0U, nb_mixed = 0U;
	unsigned int i;
	uint8_t value = 0U;

	printf("\ncriterion 6: %d rounds per phase, counting FAILURES\n", ROUNDS);
	printf("the part is 0x%02x and the neighbour is 0x%02x, chosen by the scan\n",
	       part, neighbour);

	/* One attempt each, deliberately. read_reg retries, which is right for finding out
	 * what a register holds and wrong for measuring how often a transaction succeeds. */
	for (i = 0U; i < ROUNDS; i++) {
		if (i2c_reg_read_byte(bus, part, ADXL345_REG_DEVID, &value) != 0) {
			part_alone++;
		}
	}
	for (i = 0U; i < ROUNDS; i++) {
		if (probe_addr(neighbour) != 0) {
			nb_alone++;
		}
	}
	for (i = 0U; i < ROUNDS; i++) {
		if (i2c_reg_read_byte(bus, part, ADXL345_REG_DEVID, &value) != 0) {
			part_mixed++;
		}
		if (probe_addr(neighbour) != 0) {
			nb_mixed++;
		}
	}

	printf("\n  phase          0x%02x   0x%02x\n", part, neighbour);
	printf("  each alone    %5u  %5u\n", part_alone, nb_alone);
	printf("  interleaved   %5u  %5u\n", part_mixed, nb_mixed);
	printf("\nthe criterion is the DIFFERENCE between those two rows, not either row being\n");
	printf("zero. A part on friction contacts fails sometimes with nothing else on the wire.\n");
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
	bool read_ok = false;
	unsigned int range, one_g, mag;
	int32_t ax, ay, az;
	size_t i;
	int rc;

	printf("\nthe part's registers, as whatever ran before this left them\n");
	for (i = 0U; i < ARRAY_SIZE(regs); i++) {
		uint8_t value = 0U;
		int tries = 0;

		rc = read_reg(addr, regs[i].reg, &value, &tries);
		if (rc != 0) {
			/* CARRY ON rather than returning. The first version abandoned the
			 * whole dump on the first failure, so a run reported one failed read
			 * and nothing else, and there was no way to tell a dead bus from one
			 * unlucky transaction. Seven lines of outcome are worth more than one. */
			printf("  0x%02x  %-12s silent after %d tries\n",
			       regs[i].reg, regs[i].name, tries);
			continue;
		}
		printf("  0x%02x  %-12s 0x%02x on try %d\n",
		       regs[i].reg, regs[i].name, value, tries);
		if (regs[i].reg == ADXL345_REG_DATA_FORMAT) {
			fmt = value;
			read_ok = true;
		}
	}

	if (!read_ok) {
		printf("\n  DATA_FORMAT never read, so nothing below it would mean anything\n");
		return;
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
	unsigned int neighbour = 0U;
	bool part_here;
	int found = 0;
	int disagree = 0;

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

	/* TWO PROBES, BECAUSE THE FIRST ONE IS A SUSPECT.
	 *
	 * This scan originally used a one byte read alone. It was chosen over the conventional
	 * zero-length write because some controllers refuse a zero-length transfer outright,
	 * which would report every address as absent and look like a dead bus.
	 *
	 * On Wednesday 7 October 2026 that choice came under suspicion: across four runs the
	 * scan listed 0x53 once, while a register read of the very same address a few
	 * milliseconds later succeeded four times out of four. If the probe is weaker than a
	 * register read then this scan has been under-reporting, and every conclusion drawn
	 * from a silent scan deserves re-reading.
	 *
	 * So both probes now run at every address and any disagreement between them is counted
	 * and printed. A scan that cannot report its own unreliability is one more thing to
	 * take on trust. */
	printf("scanning 0x%02x to 0x%02x, with two probes at each address\n",
	       SCAN_FIRST, SCAN_LAST);
	for (addr = SCAN_FIRST; addr <= SCAN_LAST; addr++) {
		uint8_t byte = 0U;
		bool by_read = i2c_read(bus, &byte, 1, (uint16_t)addr) == 0;
		bool by_write = i2c_write(bus, &byte, 0, (uint16_t)addr) == 0;

		if (by_read || by_write) {
			printf("  answer at 0x%02x%s\n", addr,
			       (by_read == by_write) ? "" :
			       (by_read ? "   one byte read only" : "   zero length write only"));
			found++;

			/* The first answering address that is not one of this part's two
			 * becomes the neighbour for criterion 6. Chosen by the bus rather
			 * than named here, so the diagnostic needs no list of what else is
			 * fitted and cannot be wrong about it. */
			if (neighbour == 0U && addr != ADDR_SDO_LOW && addr != ADDR_SDO_HIGH) {
				neighbour = addr;
			}
		}
		if (by_read != by_write) {
			disagree++;
		}
	}
	printf("%d address(es) answered, %d disagreement(s) between the two probes\n\n",
	       found, disagree);

	printf("the two addresses this part can have, and what each would mean\n");
	part_here = report_devid(ADDR_SDO_LOW, "SDO low, so jumper A is working");
	if (part_here) {
		dump_part(ADDR_SDO_LOW);
	}
	report_devid(ADDR_SDO_HIGH, "SDO high, so jumper A is not");

	if (part_here && neighbour != 0U) {
		interference(ADDR_SDO_LOW, (uint8_t)neighbour);
	} else if (neighbour == 0U) {
		printf("\nno second device answered, so criterion 6 has nothing to interleave\n");
	} else {
		printf("\nthe part did not identify itself, so criterion 6 would be measuring\n");
		printf("one device and a silence rather than two devices\n");
	}

	printf("\n");
	if (found == 0) {
		printf("NOTHING ANSWERED. The next question is the clock, not the leads:\n");
		printf("  rebuild with overlays/slow.overlay and compare.\n");
	}

	return 0;
}
