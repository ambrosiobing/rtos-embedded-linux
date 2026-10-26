# Criterion 5: two runs that settle the orientation without needing a document

Written at a desk on Monday 26 October 2026, after the nine hand-held runs of Tuesday 20 October
2026 placed the object somewhere other than where every instruction said. The bench log named the
next approach as reading the orientation from the vendor's zone numbering, the shield's drawing
and the photographs, and then confirming it with one run. **The reading half cannot be done from
this bench, and it turns out not to be needed.** This page says why, and replaces one confirming
run with two runs that measure the orientation outright.

## What the printed grid is, exactly

`src/main.c` prints `distance_mm[row * 8 + col]` for row 0 to 7 and col 0 to 7, in that order,
one line per row. So the printed grid's top-left cell is index 0, its top-right is index 7, its
bottom-left is index 56 and its bottom-right is index 63. **That part is settled by the source
and needs no document at all.** The open question is only which direction in the room each
printed edge corresponds to.

## Why the documentary chain stalls here

Three links were needed: the vendor's zone numbering in the sensor's own frame, the sensor's
orientation on the shield, and the shield's orientation on the board. The third is fixed by the
Arduino connector, which fits one way. The first two were sought on Monday 26 October 2026 and
neither was obtained:

| Source | Result |
|---|---|
| `vl53l8cx_api.h` at the pinned commit `c12709c3` | no comment on zone ordering; the results arrays are declared and nothing says how they map to the field |
| `vl53l8cx_api.c` at the same commit | no comment on zones, order, rows, columns, flip or mirror |
| The vendor's user manual UM3109 on its own server | did not answer, as this bench's notes already say of that server |
| One public mirror of the same manual | refused the request |

**One statement was found and it is not good enough to build on.** A search engine's summary of
that user manual says the zones increment along a row before starting a new row, that zone 0 is
at the bottom left of the detector array, and that the lens inverts the image in both axes so
that zone 0 is illuminated by a target at the top right of the scene. That is a summary of a
document this bench has not opened, so it is recorded here as a hypothesis the runs below can
agree or disagree with, and not as a premise they rest on.

## The eight possibilities, and why two runs are enough

A map from an eight by eight printed grid to an eight by eight patch of the room, with the axes
kept perpendicular, is one of eight: four rotations, each with or without a mirror. The mirror
matters here rather than being pedantry, because a lens inverts an image and a reader who assumes
it does not will be wrong half the time.

**Each run splits the field along one axis through the optical axis, and reads which half of the
printed grid went dark.**

- Run 1 fixes which printed edge is upward. Four of the eight printed rows, or four of the eight
  printed columns, read the near distance.
- Run 2 fixes which printed edge is to the viewer's left. The other axis of the grid answers.

Together they name the rotation and settle the mirror, because run 1 alone leaves a reflection
about the vertical and run 2 removes it.

## The arrangement, anchored at the sensor and at gravity

The hand-held runs failed because every instruction named a board feature off the sensor's axis,
and because a distance named in a sentence is not a distance held by a hand. Both are avoided
here by standing the board up and using gravity as the second reference.

1. **The board stands vertically**, so that the sensor looks horizontally. Nothing else about the
   board's rotation is specified, and nothing needs to be: the runs measure it.
2. **A flat surface at 1.5 to 2.5 metres** directly ahead, filling the field. The silence of a
   zone that sees nothing is a result in itself, but a filled field makes the covered half
   unmistakable, and 2.58 metres is a distance this bench has already read cleanly.
3. **A rigid card at least 300 millimetres on a side**, held in a clamp or propped, not in a
   hand. At 500 millimetres from the sensor one zone subtends roughly 50 millimetres, so 300
   millimetres covers about six zones across, which is more than the four a half needs.
4. **The card's straight edge passes through the optical axis**, which is the centre of the lens
   window and not the centre of the shield. For run 1 the edge is horizontal and the card covers
   everything above it. For run 2 the edge is vertical and the card covers everything to the left
   as seen from behind the board, looking the way the sensor looks.

The card's distance is not critical and is recorded rather than controlled: anywhere from 300 to
700 millimetres, measured from the lens window, far enough that no zone reports status 4 and near
enough that the near and far halves are plainly different numbers.

## What each run predicts, and what refutes it

**Run 1, the horizontal edge.** Exactly four of the eight printed rows, or exactly four of the
eight printed columns, read the card's distance, and the remaining four read the far surface. The
boundary falls between printed indices 3 and 4 along one axis.

*Refuted if* the split is not four and four, which means the edge did not pass through the
optical axis, or if the pattern is neither four whole rows nor four whole columns, which means
the card was not flat to the sensor or its edge was not perpendicular to gravity. Either way the
run is repeated rather than interpreted, which is the rule the nine runs of Tuesday 20 October
2026 were stopped for not following.

**Run 2, the vertical edge.** The same, on the other axis. If run 1 split rows, run 2 must split
columns; if run 1 split columns, run 2 must split rows.

*Refuted if* both runs split the same axis, which would mean one of the two cards was not
perpendicular to the other and nothing has been learnt.

**The pair together.** Eight possible maps, and the two results select exactly one. The chapter
then states the orientation with the two captures beside it.

*Refuted if* the selected map disagrees with either reading already in the record: a finger at
430 millimetres lit printed zone 56 alone, and the two pens at the sensor face put their covered
corner at printed bottom left. Those two were taken with the board flat rather than standing and
with the object in a hand, so they are weak, but a map that contradicts both deserves a third run
before it is written into the chapter.

## What this settles and what it does not

It settles criterion 5, which asks only that the grid be the right way up and that the answer be
written down before anything downstream uses it. It says nothing about the vendor's numbering in
the sensor's own frame, because the two runs measure the whole chain at once and cannot
distinguish a sensor rotated on the shield from a shield rotated on the board. If the hypothesis
above is ever confirmed from the vendor's own document, the two can be compared, and a
disagreement would locate itself in the shield rather than in the sensor.

## Sources

- `src/main.c` of this project, for the printed order, which is the one link in the chain that is
  settled by reading code rather than a document.
- `vl53l8cx_api.h` and `vl53l8cx_api.c` at commit `c12709c3`, read on Monday 26 October 2026 and
  carrying no statement about zone ordering.
- The vendor's user manual UM3109, sought on Monday 26 October 2026 and not obtained from the
  vendor's server or from one mirror. The hypothesis above is a search engine's summary of it and
  is marked as such wherever it appears.
- This project's `docs/BENCH.md` for the nine runs of Tuesday 20 October 2026, the two readings
  this page cross-checks against, and the rule that an instruction is anchored at the sensor.
