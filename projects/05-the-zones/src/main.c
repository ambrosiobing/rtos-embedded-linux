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

#include <stdbool.h>
#include <stdio.h>

#include "build_stamp.h"

#if DT_NODE_HAS_COMPAT(DT_ALIAS(zones), st_vl53l8cx)
#define READS_THE_GRID 1
#include "zones.h"
#else
/* The description that points this part at the tree's driver for the generation before it gets
 * no grid, because that driver does not bring this part up at all. One application serves two
 * descriptions and this is the one place they differ. */
#define READS_THE_GRID 0
#endif

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

#if READS_THE_GRID
/* SIXTY-FOUR DISTANCES AND SIXTY-FOUR STATUSES, PRINTED TOGETHER, which is this project's second
 * acceptance criterion in a format rather than in a sentence: an empty scene and a surface beyond
 * range both have a number in the distance, and only the status tells them apart.
 *
 * NOTHING HERE TURNS A STATUS INTO A WORD, because a mapping read off the shape of the data
 * would be a guess wearing a label. It comes from the vendor's own source or it does not appear.
 *
 * ONE FACT NOW COMES FROM THAT SOURCE, quoted from the header this project compiles against, at
 * the commit CMakeLists.txt pins, beside the `target_status` field:
 *
 *     Status indicating the measurement validity (5 & 9 means ranging OK)
 *
 * So two values mean a measurement and every other value means something that is not one. **What
 * the other values mean is still not established and is still not invented.** The only
 * documentation in that pinned tree is `Documentation/X_CUBE_TOF1.chm`, a compiled help file, and
 * no source file in it enumerates the values. The number is therefore still printed as a number,
 * with the one distinction the vendor does state marked beside it and counted underneath.
 *
 * AND THAT QUOTATION COLLIDES WITH THE ONLY DATA THIS PROJECT HAS, which is why the marking is
 * presented as the vendor's claim rather than as a verdict. The grid of Saturday 17 October 2026
 * at 15:35 was a coherent surface at about 2.56 metres, flat across its rows and rising 52
 * millimetres across its columns, and **every one of its sixty-four zones reported status 6**. So
 * did the noise grid from the boot before it. Under the sentence above, the real surface would
 * carry no marks at all.
 *
 * Two readings fit and this project cannot choose between them from here: either 6 is a state the
 * header's parenthesis does not list and the measurement is sound anyway, or that parenthesis
 * describes a field or a generation other than this one. **Both are better than a mapping
 * invented to make the output look right.** The count is printed, the conflict is printed beside
 * it, and no unmarked zone is called invalid.
 *
 * THE ORIENTATION IS UNESTABLISHED AND IS SAID TO BE. The grid is printed in the order the part
 * returns it, and which physical corner the first zone is has not been shown. That is the fifth
 * acceptance criterion and it needs a hand over one quadrant, not a reading of this file. */
/* HOW LONG TO WAIT FOR ONE RESULT BEFORE SAYING SO. Generous, because the ranging rate is the
 * vendor's default and this project has not read what that is; a bound that is too tight would
 * turn a slow part into a broken one on the console. */
#define FRAME_WAIT_MS 2000

/* The two values the vendor's header names, and nothing else is given a meaning. They are
 * written as named constants rather than as literals in the comparison so that the quotation
 * above and the code below cannot drift apart. */
#define STATUS_RANGING_OK_A 5u
#define STATUS_RANGING_OK_B 9u

static bool status_is_ranging_ok(uint8_t status)
{
	return status == STATUS_RANGING_OK_A || status == STATUS_RANGING_OK_B;
}

static void print_grid(int frame)
{
	struct zones_grid grid;
	int64_t began = k_uptime_get();
	int64_t ready_at;
	int rc;
	int row;
	int col;
	int ok;

	/* POLLING, NOT A FIXED DELAY, and the time it took is printed. The run of 15:35 showed
	 * why one frame is not enough: the first grid after a cold start was seven millimetres
	 * of noise with ten zones reading negative, and the next was a coherent surface at two
	 * and a half metres. Taking several frames in one run shows the settling instead of
	 * leaving it to be inferred from two boots. The interval between frames is also the
	 * ranging period, measured rather than assumed. */
	do {
		rc = sensor_sample_fetch(zones);
		if (rc == -EAGAIN) {
			k_msleep(5);
		}
	} while (rc == -EAGAIN && (k_uptime_get() - began) < FRAME_WAIT_MS);

	ready_at = k_uptime_get();

	if (rc == -EAGAIN) {
		printf("\n# frame %d: no result in %d ms, so none is printed\n", frame,
		       FRAME_WAIT_MS);
		return;
	}
	if (rc != 0) {
		printf("\n# frame %d: the fetch failed: %d. No grid is printed, because a value\n",
		       frame, rc);
		printf("# that might not be a measurement is not printed as one\n");
		return;
	}

	rc = zones_read(zones, &grid);
	if (rc != 0) {
		printf("\n# frame %d: the grid could not be read: %d\n", frame, rc);
		return;
	}

	printf("\n# frame %d, ready after %d ms of waiting, at %d ms since boot\n", frame,
	       (int)(ready_at - began), (int)ready_at);
	printf("# distance in millimetres, then the vendor's status, then an asterisk when that\n");
	printf("# status is 5 or 9, the only two the vendor's own header calls ranging OK. The\n");
	printf("# other values are printed as numbers because the pinned tree documents them\n");
	printf("# only in a compiled help file, and the orientation is still unestablished\n");
	printf("# silicon temperature %d C\n", grid.silicon_temp_degc);

	ok = 0;
	for (row = 0; row < ZONES_ACROSS; row++) {
		printf("#  ");
		for (col = 0; col < ZONES_ACROSS; col++) {
			int i = row * ZONES_ACROSS + col;
			bool valid = status_is_ranging_ok(grid.status[i]);

			if (valid) {
				ok++;
			}
			/* The asterisk is the vendor's distinction and not this project's opinion
			 * of the number: a zone without one has a distance that is not a
			 * measurement, whatever it reads. */
			printf(" %5d/%-2u%c", grid.distance_mm[i], grid.status[i],
			       valid ? '*' : ' ');
		}
		printf("\n");
	}
	printf("# %d of %d zones are marked, which is this frame's count of statuses 5 and 9, the\n",
	       ok, ZONES_TOTAL);
	printf("# only two the vendor's header calls ranging OK.\n");
	printf("# READ THAT COUNT AGAINST THIS: the only grid this project has recorded, a\n");
	printf("# coherent surface at about 2.56 m on 17 October 2026 at 15:35, reported status 6\n");
	printf("# in every zone, and so did the noise grid from the boot before it. A zero here is\n");
	printf("# therefore expected and is not a fault in this printer. Whether 6 is a valid\n");
	printf("# measurement the header does not list, or whether that comment describes another\n");
	printf("# field, is unresolved, so an unmarked zone is not called anything.\n");

	printf("# targets each zone saw, where zero means the zone saw nothing at all\n");
	for (row = 0; row < ZONES_ACROSS; row++) {
		printf("#  ");
		for (col = 0; col < ZONES_ACROSS; col++) {
			printf(" %u", grid.targets[row * ZONES_ACROSS + col]);
		}
		printf("\n");
	}
}
#endif

int main(void)
{
	/* THE DRIVER'S LOG ARRIVES BEFORE THIS FUNCTION AND IS FLUSHED AFTER IT, which the first
	 * run showed: initialisation happens at boot and its error landed below this
	 * application's advice, so the console read as advice first and diagnosis second. The
	 * sleep lets the deferred log drain, so that the order on the console is the order in
	 * which things happened. */
	k_msleep(200);

	printf("\np05 the zones, identification run\n");
	/* THE STAMP IS THE LINK'S AND NOT THIS FILE'S, which it used not to be. `__DATE__` here
	 * reported a time 69 minutes old for an image built minutes earlier, because an edit to
	 * the driver does not recompile main.c. scripts/stamp.py is run at every build instead,
	 * and build_stamp.h records what went wrong with the first arrangement. */
	printf("# built %s\n", build_stamp_when);
	printf("# from %s, and a stamp older than your last edit means a stale flash\n",
	       build_stamp_commit);
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
	printf("# generations differ in is read from the grid, which follows.\n");

#if READS_THE_GRID
	/* FOUR FRAMES AND NOT ONE. A processor reset does not reset this part: it keeps
	 * ranging across one, which is why the two boots of 15:35 from a single button press
	 * showed noise and then a surface. Several frames in one run make the settling visible
	 * rather than something a reader has to infer from two logs. */
	for (int frame = 1; frame <= 4; frame++) {
		print_grid(frame);
	}
#else
	printf("# This description uses the tree's driver, which does not bring this part up,\n");
	printf("# so there is no grid to read and none is invented.\n");
#endif
	return 0;
}
