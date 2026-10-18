/* projects/05-the-zones/src/zone_status.h: the only place that says which statuses mean a reading.
 *
 * ONE DEFINITION, BECAUSE THERE WERE ABOUT TO BE TWO. The application grew a pair of constants
 * for this on Sunday 18 October 2026 and the reduction needed the same pair an hour later. Two
 * copies of "which values mean a measurement" is the kind of duplication that survives until the
 * day they disagree, so it lives here and both include it.
 *
 * WHAT THE VENDOR SAYS, quoted from the header this project compiles against at the commit the
 * build file pins, beside the `target_status` field:
 *
 *     Status indicating the measurement validity (5 & 9 means ranging OK)
 *
 * That is the whole of what any source available to this project says about these values. The
 * rest of the enumeration is documented only in a compiled help file inside the pinned package,
 * so no other value is given a meaning here.
 *
 * AND IT DISAGREES WITH THE ONLY DATA THIS PROJECT HAS, which anything including this header has
 * to know. The grid of Saturday 17 October 2026 at 15:35 was a coherent surface at about 2.56
 * metres, flat across its rows and rising 52 millimetres across its columns, and **all sixty-four
 * of its zones reported status 6**. So did all sixty-four of the noise grid from the boot before
 * it. Under the sentence above, that real surface contains no valid zone at all.
 *
 * Two readings fit and this project cannot choose between them yet: either 6 is a sound state the
 * parenthesis does not list, or the parenthesis describes a different field or the generation
 * before this one. **Neither is resolved by picking the one that makes the output look right.**
 * So this header states the vendor's rule exactly as given, and every caller is written so that
 * "no valid zone" is a case it handles rather than a number it returns.
 *
 * THIS HEADER IS PLAIN C ON PURPOSE. It names no operating system, so the reduction beside it and
 * that reduction's host test share this one definition with the firmware.
 */
#ifndef P05_ZONE_STATUS_H
#define P05_ZONE_STATUS_H

#include <stdbool.h>
#include <stdint.h>

/* The two the vendor's header names, as named constants rather than literals at the comparison,
 * so the quotation above and the code cannot drift apart. */
#define ZONE_STATUS_RANGING_OK_A 5u
#define ZONE_STATUS_RANGING_OK_B 9u

static inline bool zone_status_is_ranging_ok(uint8_t status)
{
	return status == ZONE_STATUS_RANGING_OK_A || status == ZONE_STATUS_RANGING_OK_B;
}

#endif /* P05_ZONE_STATUS_H */
