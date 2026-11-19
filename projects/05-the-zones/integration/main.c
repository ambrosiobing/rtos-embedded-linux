/* projects/05-the-zones/integration/main.c: this part's grid feeding P01's machine.
 *
 * WHAT THIS EXISTS FOR is criterion 6 of docs/DESIGN.md, in the narrow form docs/CRITERION6.md
 * adopts: P01's application object is byte for byte what it was, and the file that chooses one
 * number from sixty-four is new and is not P01's. So this application links P01's machine and
 * P01's adapter unchanged, adds this project's reduction, and contains nothing else of
 * consequence. **The object comparison is the result. Nothing printed below is.**
 *
 * IT DOES NOT TOUCH THE PART. The grid is written here rather than read from the sensor, because
 * the claim is about the seam and not about the hardware: src/reduce_nearest_valid.c and
 * src/zone_status.h name no operating system and no vendor interface, so this criterion needs no
 * board, no shield, no vendor clone and no cross toolchain. Whether the reduction feeds P01 a
 * sensible number on a real scene is criteria 1 to 5 and belongs at the bench.
 *
 * THE SEAM IS THE ONE THING HERE WORTH READING, and it is two decisions.
 *
 * First, a refusal is not a distance. `reduce_nearest_valid` reports `found == false` when no
 * zone carried a status the vendor calls a reading, and this file then posts NO EVENT AT ALL.
 * docs/REDUCTION.md says it in the words this file honours: P01 "would be posted no reading event
 * at all, which is not the same as being told the range is zero". A range of zero would mean
 * somebody at the sensor face, which is the opposite of nobody there.
 *
 * Second, the types do not match and the mismatch is refused rather than cast. The reduction
 * reports `int16_t distance_mm`, because the vendor's field is signed and a zone with an object
 * at the lens reports two to ten millimetres or a small negative number. P01's event carries
 * `uint16_t range_mm`. A blind cast would turn minus three into 65533, which P01 would read as
 * nobody there, so a negative distance is refused here.
 */
#include "presence_adapter.h"   /* which includes presence.h, so the machine's types come with it */
#include "reduce_nearest_valid.h"
#include "zone_status.h"

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Leaving the simulation with a status, selected rather than assumed because the header has been
 * renamed across releases. This repeats P01's main.c rather than sharing it: sharing would mean a
 * new file in P01, and criterion 6 is the claim that P01 is unchanged. */
#if defined(__has_include)
#  if __has_include(<nsi_main.h>)
#    include <nsi_main.h>
#    define ZONES_EXIT(code) nsi_exit(code)
#  elif __has_include(<posix_board_if.h>)
#    include <posix_board_if.h>
#    define ZONES_EXIT(code) posix_exit(code)
#  endif
#endif
#ifndef ZONES_EXIT
#  include <stdlib.h>
#  define ZONES_EXIT(code) exit(code)
#endif

#define ZONES 64u

/* THE TWO STATUS VALUES USED BELOW ARE LITERALS ON PURPOSE. src/zone_status.h names the vendor's
 * two ranging-OK values as constants and says in as many words that no other value is given a
 * meaning there, because the rest of the enumeration is documented only inside a compiled help
 * file in the pinned package. What 255 and 4 were observed to mean is recorded in that header
 * from seven boots on Tuesday 20 October 2026: 255 is a zone that saw no target, and 4 is the
 * whole field with an object at the sensor face. Giving them names here would create the second
 * definition that header exists to prevent. */
#define OBSERVED_NO_TARGET    255u
#define OBSERVED_AT_THE_FACE  4u

/* The adapter calls this on every state change and the application supplies it. Nothing is
 * driven here, exactly as in P01's own main under native_sim. */
void presence_indicators(presence_state_t state)
{
	ARG_UNUSED(state);
}

/* ----------------------------------------------------------------- the seam, and only the seam
 *
 * One grid in, at most one event out. It returns whether an event was posted and reports nothing
 * itself, because a function that both decides and prints is two functions.
 */
static bool post_nearest_as_reading(const int16_t *distance_mm, const uint8_t *status,
				    uint8_t *out_zone, int16_t *out_mm)
{
	const struct reduce_nearest_valid_result r =
		reduce_nearest_valid(distance_mm, status, ZONES);

	if (!r.found || r.distance_mm < 0) {
		return false;
	}

	const presence_event_t ev = {
		.kind = PRESENCE_EV_READING,
		.u.range_mm = (uint16_t)r.distance_mm,
	};

	*out_zone = r.zone;
	*out_mm = r.distance_mm;
	return presence_adapter_post(&ev);
}

static void fill(int16_t *distance_mm, uint8_t *status, int16_t mm, uint8_t st)
{
	for (unsigned int i = 0; i < ZONES; i++) {
		distance_mm[i] = mm;
		status[i] = st;
	}
}

int main(void)
{
	int16_t distance_mm[ZONES];
	uint8_t status[ZONES];
	uint8_t zone = 0;
	int16_t mm = 0;

	printf("\np05 integration: this part's grid into P01's machine\n");
	printf("# the grid is written in this file and not read from the part, which\n");
	printf("# docs/CRITERION6.md explains: this build exists so that P01's object can be\n");
	printf("# compared with P01's own, and that comparison is the result rather than anything\n");
	printf("# printed here\n");

	if (!presence_adapter_init()) {
		printf("FAILED: the adapter did not start\n");
		ZONES_EXIT(1);
		return 1;
	}

	/* One: a surface at 1200 mm, every zone a reading. P01 must be given 1200. */
	fill(distance_mm, status, 1200, ZONE_STATUS_RANGING_OK_A);
	if (!post_nearest_as_reading(distance_mm, status, &zone, &mm)) {
		printf("FAILED: a grid of valid readings posted nothing\n");
		ZONES_EXIT(1);
		return 1;
	}
	printf("# a full grid at 1200 mm posted %d mm from zone %u\n", (int)mm, (unsigned int)zone);

	/* Two: nothing anywhere. P01 must be given no event, which is the whole of the
	 * distinction docs/REDUCTION.md exists to keep. */
	fill(distance_mm, status, 0, OBSERVED_NO_TARGET);
	if (post_nearest_as_reading(distance_mm, status, &zone, &mm)) {
		printf("FAILED: a grid with no target posted a reading\n");
		ZONES_EXIT(1);
		return 1;
	}
	printf("# a grid with no target posted nothing, which is not a range of zero\n");

	/* Three: an object at the lens, reported as a small negative distance. A blind cast
	 * would make minus three into 65533, which P01 reads as nobody there. */
	fill(distance_mm, status, -3, OBSERVED_AT_THE_FACE);
	if (post_nearest_as_reading(distance_mm, status, &zone, &mm)) {
		printf("FAILED: a negative distance was posted as a range\n");
		ZONES_EXIT(1);
		return 1;
	}
	printf("# a negative distance posted nothing rather than 65533 mm\n");

	presence_adapter_sleep_ms(50);

	printf("PASSED: 0 failure(s)\n");
	ZONES_EXIT(0);
	return 0;
}
