/* projects/05-the-zones/src/reduce_nearest_valid.h: sixty-four numbers to one, by a named policy.
 *
 * THE POLICY IS IN THE FILENAME BECAUSE IT IS A PRODUCT DECISION. docs/DESIGN.md argues this at
 * length and the short form is that reducing a grid to a single distance is not a mechanism:
 *
 *     the nearest valid zone        suits a presence machine that should respond to anything
 *                                   entering the field, and loses the difference between a wall
 *                                   at two metres and a person at two metres
 *     a named zone, the centre say  suits a device pointed at a doorway, and loses the other
 *                                   sixty-three
 *     how many zones are occupied   suits the question this volume's product spine asks, how much
 *     below a threshold             of a room is in use, and loses where
 *
 * **The driver chooses none of them.** It offers the grid. This file is the first of those three,
 * written because P01's state machine compares one distance against one maximum, and it is a
 * separate file with its policy in its name so that a reader can disagree with it in one place
 * rather than find it inside a device driver.
 *
 * NO THRESHOLD LIVES HERE, which P01's own contract requires: "a threshold found in an adapter
 * means the adapter is wrong, not the table. Every number lives in presence_settings_t and every
 * decision is a row." This file produces a distance and compares nothing.
 *
 * THE ONE THING THIS INTERFACE REFUSES TO DO, and it is the reason the result is a structure
 * rather than an int16_t. When no zone carries a status the vendor calls a reading, there is no
 * distance, and the obvious shapes for saying so are all wrong in the same direction:
 *
 *   return 0                 P01's test is `range_mm <= range_mm_max`, so zero is the closest
 *                            possible object and reads as something touching the sensor. An
 *                            empty answer would become a presence
 *   return INT16_MAX         reads as a real surface at 32.7 metres, which is beyond this part
 *                            and would be published as a measurement by anything downstream
 *   return a negative        the part itself returns negatives, ten of them in the noise grid of
 *                            Saturday 17 October 2026, so this collides with real data
 *
 * So `found` is a separate field and a caller that ignores it gets a distance that means nothing.
 * The caller's correct response to `found == false` is to post no reading event at all, which
 * leaves P01's hold to expire on its own timer, a case its table already has rows for.
 *
 * PLAIN C AND NO OPERATING SYSTEM, so this and its host test compile anywhere. The firmware passes
 * the arrays out of `struct zones_grid`; nothing here knows that structure exists.
 */
#ifndef P05_REDUCE_NEAREST_VALID_H
#define P05_REDUCE_NEAREST_VALID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct reduce_nearest_valid_result {
	/* False means **there is no reading**, not that the reading is far away. Read this before
	 * distance_mm, which is left at zero and is not a distance. */
	bool found;

	/* Millimetres, meaningful only when found is true. */
	int16_t distance_mm;

	/* Which zone it came from, in the order the part returns them. Carried because the grid's
	 * orientation is this project's fifth acceptance criterion and is not yet established: a
	 * reader checking which physical corner is which needs the index, not just the value. */
	uint8_t zone;

	/* How many zones carried a status the vendor calls a reading. This is what explains a
	 * refusal, and it is the number to print when found is false: zero of sixty-four is a
	 * different situation from a bus failure and the two must not look alike. */
	uint8_t valid_zones;
};

/* Reduce a grid to the nearest zone whose status the vendor calls a reading.
 *
 * distance_mm and status are parallel arrays of `count` entries. A null pointer or a zero count
 * returns found == false with valid_zones == 0, because a caller that passes nothing has asked
 * nothing and must not receive a distance.
 *
 * Ties go to the lower zone index, which is arbitrary and is stated rather than left to the
 * order of a loop: two zones at the same distance is the normal case for a flat surface, and a
 * reader comparing two runs needs the choice to be repeatable.
 */
struct reduce_nearest_valid_result reduce_nearest_valid(const int16_t *distance_mm,
						       const uint8_t *status,
						       size_t count);

#endif /* P05_REDUCE_NEAREST_VALID_H */
