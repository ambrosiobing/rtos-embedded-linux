/* projects/05-the-zones/host/test_reduce_nearest_valid.c: the reduction, on input with known answers.
 *
 * WHY THIS IS A HOST TEST AND NOT A BENCH READING. The reduction is plain C over two arrays, so
 * every one of its answers is known by construction and none of them needs the board. That is the
 * method this volume already uses for P04's reduction: prove the thing on input whose answers are
 * arithmetic, then break it on purpose and confirm the suite notices.
 *
 * THE CASES THIS REFUSES MATTER MORE THAN THE CASES IT ACCEPTS, and two in particular:
 *
 *   all sixty-four zones at status 6   which is the only grid this project has ever recorded, a
 *                                      coherent surface at about 2.56 metres. Under the vendor's
 *                                      own rule it contains no valid zone, so the reduction must
 *                                      report no reading rather than a distance
 *   an invalid zone that is nearer     the whole point of the status field. A nearer zone the
 *                                      vendor does not call a reading must not win
 *
 * EXPECTED VALUES ARE WRITTEN OUT, never computed by calling the thing under test. A test that
 * asks the implementation what the answer is agrees with itself; this volume has that mistake on
 * file as an assertion written to match its own sentence rather than its input.
 *
 *   cc -std=c11 -Wall -Wextra -Werror -I../src -o test_reduce_nearest_valid \
 *      test_reduce_nearest_valid.c ../src/reduce_nearest_valid.c && ./test_reduce_nearest_valid
 *
 * or `make check` in this directory, and `make mutants` to confirm the suite can fail at all.
 */
#include "reduce_nearest_valid.h"
#include "zone_status.h"

#include <stdio.h>

/* Unsigned, so every `i < ZONES` compares two unsigned values and no signed-compare warning
 * can appear under -Wextra -Werror, which this suite is built with. */
#define ZONES 64u

static int failures;
static int checks;

static void expect(int condition, const char *what)
{
	checks++;
	if (!condition) {
		failures++;
		printf("  FAILED: %s\n", what);
	}
}

/* A grid builder, so each case states only what it changes. Everything starts at a status the
 * vendor does not call a reading, which is the conservative default: a case that forgets to set a
 * status gets no reading rather than an accidental pass. */
static void grid_init(int16_t *mm, uint8_t *st, int16_t fill_mm, uint8_t fill_status)
{
	size_t i;

	for (i = 0u; i < ZONES; i++) {
		mm[i] = fill_mm;
		st[i] = fill_status;
	}
}

static void case_nothing_passed(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("a caller that passes nothing gets no reading\n");
	grid_init(mm, st, 1000, ZONE_STATUS_RANGING_OK_A);

	r = reduce_nearest_valid(NULL, st, ZONES);
	expect(!r.found, "null distances must give no reading");
	expect(r.valid_zones == 0u, "null distances must report zero valid zones");

	r = reduce_nearest_valid(mm, NULL, ZONES);
	expect(!r.found, "null statuses must give no reading");

	r = reduce_nearest_valid(mm, st, 0u);
	expect(!r.found, "a zero count must give no reading");
	expect(r.valid_zones == 0u, "a zero count must report zero valid zones");
}

static void case_the_only_grid_this_project_has(void)
{
	/* The recorded grid of Saturday 17 October 2026 at 15:35: distances between 2540 and 2640
	 * millimetres, every zone one target, **every zone status 6**. Reproduced here as the
	 * shape rather than the exact sixty-four values, because what is being tested is the
	 * status rule and not the surface. */
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];
	size_t i;

	printf("the only grid ever recorded here, all status 6, yields no reading\n");
	grid_init(mm, st, 0, 6u);
	for (i = 0u; i < ZONES; i++) {
		mm[i] = (int16_t)(2540 + (int)i);
	}

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(!r.found, "status 6 is not a status the vendor calls a reading, so no reading");
	expect(r.valid_zones == 0u, "zero of sixty-four zones are valid under the vendor's rule");
	expect(r.distance_mm == 0, "no reading leaves no distance behind to be misread");
}

static void case_one_valid_zone(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("one valid zone is the answer, wherever it sits\n");
	grid_init(mm, st, 9999, 6u);
	mm[37] = 1234;
	st[37] = ZONE_STATUS_RANGING_OK_A;

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "one valid zone is a reading");
	expect(r.distance_mm == 1234, "the distance is that zone's");
	expect(r.zone == 37u, "the zone index is carried, for the orientation question");
	expect(r.valid_zones == 1u, "one valid zone is counted as one");
}

static void case_status_nine_counts_too(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("status 9 is a reading as much as status 5\n");
	grid_init(mm, st, 9999, 6u);
	mm[2] = 800;
	st[2] = ZONE_STATUS_RANGING_OK_B;

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "status 9 is a reading");
	expect(r.distance_mm == 800, "and its distance is used");
}

static void case_nearest_of_several_wins(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("the nearest valid zone wins, not the first\n");
	grid_init(mm, st, 9999, 6u);
	mm[5] = 3000;  st[5] = ZONE_STATUS_RANGING_OK_A;
	mm[9] = 1500;  st[9] = ZONE_STATUS_RANGING_OK_A;
	mm[50] = 2000; st[50] = ZONE_STATUS_RANGING_OK_B;

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "three valid zones are a reading");
	expect(r.distance_mm == 1500, "1500 is nearer than 2000 and 3000");
	expect(r.zone == 9u, "and zone 9 is where it came from");
	expect(r.valid_zones == 3u, "all three valid zones are counted");
}

static void case_a_nearer_invalid_zone_must_not_win(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("a nearer zone the vendor does not call a reading must not win\n");
	grid_init(mm, st, 9999, 6u);
	mm[3] = 100;   st[3] = 4u;                          /* nearer, and not a reading */
	mm[8] = 2500;  st[8] = ZONE_STATUS_RANGING_OK_A;    /* further, and a reading */

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "the valid zone is a reading");
	expect(r.distance_mm == 2500, "the nearer invalid zone is ignored, which is the whole "
				      "reason the status field exists");
	expect(r.zone == 8u, "and the answer came from the valid zone");
	expect(r.valid_zones == 1u, "the invalid zone is not counted as valid");
}

static void case_ties_go_to_the_lower_index(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("a tie keeps the lower zone index, repeatably\n");
	grid_init(mm, st, 9999, 6u);
	mm[11] = 1800; st[11] = ZONE_STATUS_RANGING_OK_A;
	mm[12] = 1800; st[12] = ZONE_STATUS_RANGING_OK_A;
	mm[40] = 1800; st[40] = ZONE_STATUS_RANGING_OK_A;

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "a tie is still a reading");
	expect(r.zone == 11u, "the lowest index of the tied zones");
	expect(r.valid_zones == 3u, "and all three are counted");
}

static void case_negatives_are_reported_not_hidden(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];

	printf("a valid zone with a negative distance is reported as it stands\n");
	grid_init(mm, st, 9999, 6u);
	mm[20] = -2; st[20] = ZONE_STATUS_RANGING_OK_A;
	mm[21] = 500; st[21] = ZONE_STATUS_RANGING_OK_A;

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "it is a reading because the status says so");
	expect(r.distance_mm == -2, "and the value is not corrected or filtered here");
	expect(r.zone == 20u, "the negative zone is nearest and is chosen");
}

static void case_every_zone_valid(void)
{
	struct reduce_nearest_valid_result r;
	int16_t mm[ZONES];
	uint8_t st[ZONES];
	size_t i;

	printf("sixty-four valid zones are counted as sixty-four\n");
	grid_init(mm, st, 0, ZONE_STATUS_RANGING_OK_A);
	for (i = 0u; i < ZONES; i++) {
		mm[i] = (int16_t)(4000 - (int)i);  /* the last zone is nearest */
	}

	r = reduce_nearest_valid(mm, st, ZONES);
	expect(r.found, "a full grid of readings is a reading");
	expect(r.valid_zones == 64u, "and all sixty-four are counted");
	expect(r.zone == 63u, "the nearest is the last zone, which the loop must reach");
	expect(r.distance_mm == 3937, "4000 minus 63");
}

int main(void)
{
	case_nothing_passed();
	case_the_only_grid_this_project_has();
	case_one_valid_zone();
	case_status_nine_counts_too();
	case_nearest_of_several_wins();
	case_a_nearer_invalid_zone_must_not_win();
	case_ties_go_to_the_lower_index();
	case_negatives_are_reported_not_hidden();
	case_every_zone_valid();

	printf("\n%d check(s), %d failure(s)\n", checks, failures);
	if (failures == 0) {
		printf("every case behaved\n");
	}
	return failures == 0 ? 0 : 1;
}
