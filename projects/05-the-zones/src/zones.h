/* projects/05-the-zones/src/zones.h: how an application reads sixty-four zones from this driver.
 *
 * WHY THIS EXISTS AT ALL, which docs/DESIGN.md argues at length and this states in three lines.
 * The sensor interface hands back one value and this part returns sixty-four, each with a status.
 * Reducing them to one is a policy: nearest valid zone, a named zone, or how many are occupied
 * below a threshold are three different products. **The driver does not choose**, so it offers
 * the grid and a reduction, when this project writes one, is a separate file with the policy in
 * its name.
 *
 * THE STATUS IS NOT OPTIONAL AND IS NOT A DECORATION. A zone that saw nothing and a zone that
 * saw a surface beyond range both have a distance in them, and only the status tells them apart.
 * That is this project's second acceptance criterion, and it is the reason this structure carries
 * the two together rather than offering distances and letting a caller ask for statuses if it
 * remembers to.
 */
#ifndef P05_ZONES_H
#define P05_ZONES_H

#include <zephyr/device.h>
#include <zephyr/types.h>

#define ZONES_ACROSS 8
#define ZONES_TOTAL  (ZONES_ACROSS * ZONES_ACROSS)

struct zones_grid {
	/* Millimetres, as the vendor's library reports them, and meaningless on their own:
	 * read the status beside each one first. */
	int16_t distance_mm[ZONES_TOTAL];

	/* The vendor's own status for the zone. **This project has not yet established what each
	 * value means**, so nothing here converts it to a word, and the application prints the
	 * number. A mapping invented from the shape of the data would be the kind of claim this
	 * volume exists not to publish; it comes from ST's documentation or not at all. */
	uint8_t status[ZONES_TOTAL];

	/* How many targets the zone saw. With one target per zone configured, which
	 * src/platform.h argues for, this is zero or one and is the cheapest check that a zone
	 * saw anything at all. */
	uint8_t targets[ZONES_TOTAL];

	/* The part's own temperature, which the vendor returns with every result. Kept because
	 * it is free and because a reading that drifts with it would otherwise be a mystery. */
	int8_t silicon_temp_degc;
};

/* Fills the grid from the most recent ranging result. Returns 0, or a negative error: -EAGAIN
 * when no result is ready yet, which is not a fault and is how a caller paces itself. */
int zones_read(const struct device *dev, struct zones_grid *grid);

#endif /* P05_ZONES_H */
