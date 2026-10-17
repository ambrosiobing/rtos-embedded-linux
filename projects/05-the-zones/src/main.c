/* projects/05-the-zones/src/main.c: the identification run, and the bus scan that diagnoses it.
 *
 * WHAT THIS FILE IS FOR, AND WHAT IT DELIBERATELY DOES NOT DO. It scans the bus, asks whether
 * the device behind one devicetree alias came up, reports both, and stops. It does not read the
 * grid. That is not an oversight: it is the first acceptance criterion of this project standing
 * on its own, and it is the one run that can be made before the grid's read interface has been
 * read rather than guessed.
 *
 * THE SCAN WAS ADDED AFTER THE FIRST RUN, on Saturday 17 October 2026 at 11:16, and the reason
 * is worth the paragraph. That run printed `NOT READY` and the driver logged `Failed to
 * initialize the sensor (255)`, which is the vendor library's generic error. The project had
 * written three candidate causes down in advance, and the generic error distinguishes none of
 * them: a part at a different address, a part held in its low-power state and a part whose
 * interface jumper selects the other bus all fail identically from inside a driver that only
 * ever talks to one address.
 *
 * A SCAN SEPARATES THEM IN ONE RUN, which is why it is here rather than a second guess at the
 * address:
 *
 *   nothing answers anywhere          the part is not on this bus at all. Either it is not
 *                                     powered, or it is held in its low-power state, or the
 *                                     jumper selects the four-wire bus. No address is wrong,
 *                                     because no address is right
 *   something answers at the address  the overlay is right and the failure is inside
 *     the overlay names                initialisation, which points at the firmware image or at
 *                                     a genuine difference between the two generations
 *   something answers elsewhere       the overlay's address is wrong and the scan has just
 *                                     printed the right one
 *   everything answers               the scan itself is void on this controller, which it says
 *                                     rather than reporting 112 sensors
 *
 * THE LAST ROW IS THE POINT OF A SCAN THAT CAN FAIL. A probe is a zero-length write and a
 * controller that does not implement one can report success for every address. A scan that
 * cannot tell that about itself would hand back a list of 112 devices and look authoritative.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <stdio.h>

/* The application's only piece of hardware knowledge, and it names no bus, no address and no
 * pin. P03 established that shape. The bus below is derived from the part rather than named
 * again, so the scan cannot drift to a different controller than the device sits on. */
static const struct device *const zones = DEVICE_DT_GET(DT_ALIAS(zones));
static const struct device *const bus = DEVICE_DT_GET(DT_BUS(DT_ALIAS(zones)));

#define ZONES_ADDR DT_REG_ADDR(DT_ALIAS(zones))

/* The seven-bit range a device may occupy, with the reserved blocks at each end left out, which
 * is the range the operating system's own shell scans. */
#define FIRST_ADDR 0x08
#define LAST_ADDR  0x77

static int scan_bus(void)
{
	int found = 0;
	uint16_t addr;
	uint8_t byte = 0;

	printf("\n# bus scan on %s, addresses 0x%02x to 0x%02x\n",
	       bus->name, FIRST_ADDR, LAST_ADDR);

	for (addr = FIRST_ADDR; addr <= LAST_ADDR; addr++) {
		struct i2c_msg probe = {
			.buf = &byte,
			.len = 0u,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		};

		if (i2c_transfer(bus, &probe, 1, addr) == 0) {
			printf("#   0x%02x answers%s\n", addr,
			       addr == ZONES_ADDR ? ", which is the address the overlay names" : "");
			found++;
		}
	}

	if (found == 0) {
		printf("#   nothing answered. The part is not on this bus: not powered, held in\n");
		printf("#   its low-power state, or the jumper selects the four-wire bus\n");
	} else if (found > (LAST_ADDR - FIRST_ADDR) / 2) {
		/* THE SCAN SAYING IT IS VOID, which is the whole reason it counts. */
		printf("#   %d of %d addresses answered, so THIS SCAN IS VOID: a controller that\n",
		       found, LAST_ADDR - FIRST_ADDR + 1);
		printf("#   does not implement a zero-length write reports success for every\n");
		printf("#   address. Nothing above is evidence of a device\n");
		return -1;
	}

	printf("#   %d device(s) answered\n", found);
	return found;
}

int main(void)
{
	/* THE DRIVER'S LOG ARRIVES BEFORE THIS FUNCTION AND IS FLUSHED AFTER IT, which the first
	 * run showed: initialisation happens at boot and its error landed below this
	 * application's advice, so the console read as advice first and diagnosis second. The
	 * sleep lets the deferred log drain, so that the order on the console is the order in
	 * which things happened. */
	k_msleep(200);

	printf("\np05 the zones, identification run\n");
	printf("# built %s %s, and a stamp older than your last edit means a stale flash\n",
	       __DATE__, __TIME__);
	printf("# the overlay names address 0x%02x on %s\n", (unsigned int)ZONES_ADDR, bus->name);
	printf("# this run asks two questions: what is on the bus, and did the device come up\n");
	printf("# it does not read the grid, and ../docs/DESIGN.md says why not yet\n");

	if (!device_is_ready(bus)) {
		printf("\nNOT READY %s, the bus controller itself\n", bus->name);
		printf("# nothing below would mean anything, so the scan is not run\n");
		return 1;
	}

	(void)scan_bus();

	if (!device_is_ready(zones)) {
		/* NOT A GRID OF ZEROES, which is the failure criterion 1 exists to prevent. The
		 * scan above is now the diagnosis and this block only says how to read it. */
		printf("\nNOT READY %s\n", zones->name);
		printf("# read the scan above, not this list:\n");
		printf("#   nothing answered     the part is not on the bus. Power, the low-power\n");
		printf("#                        line, or the interface jumper\n");
		printf("#   0x%02x answered         the address is right and initialisation failed,\n",
		       (unsigned int)ZONES_ADDR);
		printf("#                        which points at the firmware image or at a real\n");
		printf("#                        difference between the two generations\n");
		printf("#   another address      the overlay is wrong and the scan printed the\n");
		printf("#                        right answer\n");
		printf("# No reading is printed, because a value that might not be a measurement\n");
		printf("# is not printed as one\n");
		return 1;
	}

	/* THE NODE'S NAME SAYS WHOSE DRIVER THIS IS, and this text used not to, which is a
	 * defect this run found in the application rather than in the firmware. It read that
	 * the driver the tree already carries binds and initialises this part, which is true
	 * of the description pointing the part at the tree's driver for the previous
	 * generation and false of the one pointing it at this project's own, which is the
	 * description that printed it. One application serving two descriptions may not carry
	 * a sentence that is only true of one of them. */
	printf("\nREADY %s\n", zones->name);
	printf("# a driver bound this node and initialised the part, and the node's name above\n");
	printf("# says which: vl53l5cx is the tree's, for the generation before this one, and\n");
	printf("# vl53l8cx is this project's own.\n");
	printf("# This is the beginning of the question rather than the end: what the two\n");
	printf("# generations differ in is read from the grid, which this run does not take.\n");
	return 0;
}
