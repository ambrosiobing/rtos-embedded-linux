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
 * IT SEEMED TO DISAGREE WITH THE ONLY DATA THIS PROJECT HAD, and the disagreement was resolved by
 * more data rather than by choosing. The grid of Saturday 17 October 2026 at 15:35, a coherent
 * surface at about 2.56 metres, reported status 6 in all sixty-four zones, and so did the noise
 * grid from the boot before it; under the sentence above neither contained a valid zone. On
 * Sunday 18 October 2026 this header said two readings fit and refused to pick.
 *
 * WHAT SEVEN BOOTS ON TUESDAY 20 OCTOBER 2026 ESTABLISHED, three of them after the prediction was
 * written:
 *
 *   6     the first frame after ranging starts, every zone, every boot. Both Saturday grids were
 *         first frames, which is why they were 6 throughout. A first frame can be a coherent
 *         surface or sixty-four zones of noise and the status does not say which, so the vendor
 *         is right to exclude it and a caller refusing frame 1 is protected rather than pedantic
 *   5     every frame from the second on, with a flat surface in view. The vendor's rule and the
 *         data agree from frame 2 onward
 *   9     seen once, on one zone of one frame, the vendor's other ranging OK value
 *   255   a zone that saw no target: in every zone reporting it the target count was zero, and
 *         in no other zone. The distance printed beside it is a number and not a measurement,
 *         which is this project's second acceptance criterion observed rather than argued
 *   4     the whole field, with an object at the sensor face, two to ten millimetres away
 *   10, 13, 14   seen on zones at the edge of an object or on first frames; contexts noted in
 *         docs/BENCH.md, meanings still not read from a source
 *
 * So the valid set below stays exactly the vendor's two. Every caller is still written so that
 * "no valid zone" is a case it handles rather than a number it returns, and that design was
 * exercised eight times out of eight with an object at the sensor face: the reduction gave the
 * presence machine nothing rather than a range of zero.
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
