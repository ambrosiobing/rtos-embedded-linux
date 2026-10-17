/* projects/05-the-zones/src/main.c: the identification run, and nothing more yet.
 *
 * WHAT THIS FILE IS FOR, AND WHAT IT DELIBERATELY DOES NOT DO. It asks whether the device
 * behind one devicetree alias came up, reports the answer, and stops. It does not read the
 * grid. That is not an oversight and it is not laziness: it is the first acceptance criterion
 * of this project standing on its own, and it is the one run that can be made before the grid's
 * read interface has been read rather than guessed.
 *
 * THE CHEAP ATTEMPT LIVES HERE. Before a driver is written for the part on this bench, the part
 * is pointed at the driver the operating system already carries for the generation before it,
 * and what happens is reported either way. Three outcomes are all useful and all different:
 *
 *   the device is ready        the existing driver binds and initialises this part, and the
 *                              question becomes how far that goes: whether the grid it returns
 *                              is right, and what the two generations actually differ in
 *   the device is not ready    the driver bound and initialisation failed. The driver's own log
 *                              says why, which is the reason CONFIG_LOG is on in a project that
 *                              measures nothing, and the likeliest why is the firmware image
 *   the build fails            the description is wrong, which is a cheaper fault to find than
 *                              either of the above
 *
 * THE FIRMWARE IMAGE IS THE EXPECTED FIRST FAULT and it is not a wiring problem. This part
 * uploads a binary image at initialisation; the operating system's rules do not admit a blob in
 * its source, so the vendor library ships with that buffer removed and the image is fetched into
 * the workspace with `west blobs fetch hal_st`. A run that skips it fails exactly here, at the
 * readiness check, and would read as an absent part. ../docs/DESIGN.md says so at more length,
 * and this comment exists so that the person reading the console does not have to find it.
 *
 * CRITERION 1 IS THIS FILE'S WHOLE CONTENT: the device comes up, and failure is reported as
 * failure. The refutation the project wrote for it is that an absent sensor must not report
 * zeroes, because a grid of zeroes looks like an empty room and is more convincing than a single
 * zero for having structure. The cheapest way to honour that is to own no grid at all until the
 * device has said it is ready, which is why there is no buffer in this file.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <stdio.h>

/* The only piece of hardware knowledge in the file, and it names no bus, no address and no pin.
 * P03 established that shape and the reason holds here: the overlay hangs the part off a
 * controller and ends with this alias, so this line is the whole of the contract. */
static const struct device *const zones = DEVICE_DT_GET(DT_ALIAS(zones));

int main(void)
{
	printf("p05 the zones, identification run\n");
	printf("# built %s %s, and a stamp older than your last edit means a stale flash\n",
	       __DATE__, __TIME__);

	/* The build stamp is P04's lesson arriving here rather than being re-learned. On Friday
	 * 9 October 2026 a stale image on this same board read as a reproduction for an hour,
	 * because the application's own name cannot see a commit. The board is shared between two
	 * volumes, so every image on it says when it was built. */

	printf("# this run asks one question: did the device come up\n");
	printf("# it does not read the grid, and ../docs/DESIGN.md says why not yet\n");

	if (!device_is_ready(zones)) {
		/* NOT A GRID OF ZEROES, which is the failure criterion 1 exists to prevent. The
		 * driver's own log above this line is the diagnosis; this line only says which
		 * device and what to try first. */
		printf("\nNOT READY %s\n", zones->name);
		printf("# the driver's log above names the cause. In order of likelihood:\n");
		printf("#   1. the firmware image is absent: run `west blobs fetch hal_st`\n");
		printf("#      in the workspace and build again. This part uploads an image at\n");
		printf("#      initialisation and the tree does not ship it\n");
		printf("#   2. the part does not answer at the address the overlay names\n");
		printf("#   3. the shield is not fitted, or not the shield this overlay describes\n");
		printf("# No reading is printed, because a value that might not be a measurement\n");
		printf("# is not printed as one\n");
		return 1;
	}

	printf("\nREADY %s\n", zones->name);
	printf("# the driver the tree already carries binds and initialises this part.\n");
	printf("# That is the cheap attempt answered in the affirmative, and it is the\n");
	printf("# beginning of the question rather than the end: what the two generations\n");
	printf("# differ in is read from the grid, which this run does not take.\n");
	return 0;
}
