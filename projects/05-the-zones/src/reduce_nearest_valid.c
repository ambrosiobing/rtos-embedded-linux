/* projects/05-the-zones/src/reduce_nearest_valid.c: the nearest zone the vendor calls a reading.
 *
 * The interface, the policy and the three shapes this deliberately does not return are all in
 * reduce_nearest_valid.h. What is here is the loop and two decisions worth stating beside it.
 *
 * NEGATIVE DISTANCES ARE NOT FILTERED OUT, and that is deliberate rather than an omission. The
 * first grid of Saturday 17 October 2026 at 15:35 had ten of sixty-four zones reading between
 * minus two and plus five millimetres, every one of them claiming a target. A reduction that
 * silently dropped negatives would have turned that grid into a plausible answer instead of an
 * obvious one, and **the status is the field that is supposed to carry validity**. If the vendor's
 * status admits a zone, this reports what that zone said. Correcting the part's arithmetic is not
 * this file's job and hiding it is nobody's.
 *
 * THE COUNT IS CARRIED EVEN WHEN NOTHING IS FOUND, because zero valid zones out of sixty-four and
 * a grid that was never read are different situations that produce the same `found == false`. The
 * caller can tell them apart only if the count comes back.
 */
#include "reduce_nearest_valid.h"

#include "zone_status.h"

struct reduce_nearest_valid_result reduce_nearest_valid(const int16_t *distance_mm,
						       const uint8_t *status,
						       size_t count)
{
	/* EVERY FIELD NAMED, rather than `= { 0 }`. A brace-zero of a multi-member struct is what
	 * `-Wmissing-field-initializers` exists to complain about, the warning is in `-Wextra`, and
	 * this project builds with warnings as errors. Four assignments cost nothing and cannot be
	 * read two ways. */
	struct reduce_nearest_valid_result out;
	size_t i;

	out.found = false;
	out.distance_mm = 0;
	out.zone = 0u;
	out.valid_zones = 0u;

	/* A caller that passed nothing has asked nothing. Returning found == false here rather
	 * than reading through a null pointer is the same rule as everywhere else in this
	 * project: a value that might not be a measurement is not produced as one. */
	if (distance_mm == NULL || status == NULL || count == 0u) {
		return out;
	}

	for (i = 0u; i < count; i++) {
		if (!zone_status_is_ranging_ok(status[i])) {
			continue;
		}

		if (out.valid_zones < UINT8_MAX) {
			out.valid_zones++;
		}

		/* Strictly less than, so a tie keeps the zone already chosen, which is the lower
		 * index. The header states that choice rather than leaving it to this comparison
		 * being read correctly. */
		if (!out.found || distance_mm[i] < out.distance_mm) {
			out.found = true;
			out.distance_mm = distance_mm[i];
			out.zone = (uint8_t)i;
		}
	}

	return out;
}
