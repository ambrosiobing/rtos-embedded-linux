# Sixty-four numbers to one, and whose decision that is

Written Sunday 18 October 2026. [docs/DESIGN.md](DESIGN.md) argued before any code that reducing
this part's grid to a single distance is a policy rather than a mechanism, and that the driver
must not choose. This page is the record that argument asked for: the policy, in its own file,
with its name on the front and its cost written down.

## Why a reduction exists at all

P01's state machine takes one number. Its whole comparison is `range_mm <= range_mm_max`, and its
contract is strict about where numbers live: "a threshold found in an adapter means the adapter is
wrong, not the table. Every number lives in `presence_settings_t` and every decision is a row."

This part returns sixty-four distances, each with a status. Something has to choose. The sixth
acceptance criterion says P01 is unchanged and only its range source differs, and the honest
reading of that is narrower than it sounds: **P01's application is unchanged, and the thing that
chooses one number from sixty-four is new and is not P01's.**

## The three candidates, and which this is

| A reduction | What it suits | What it loses |
|---|---|---|
| **the nearest valid zone** | a presence machine that should respond to anything entering the field, which is P01 | a wall at two metres and a person at two metres are the same answer |
| a named zone, the centre say | a device pointed at a doorway | everything the other sixty-three zones saw |
| how many zones are occupied below a threshold | how much of a room is in use, which is the question this volume's product spine asks | where |

`src/reduce_nearest_valid.c` is the first of the three. The other two are not written, because
nothing in this volume needs them yet and a reduction written for an imagined consumer is a
policy nobody has had to defend.

**The cost of this one is in the table and is not a detail.** A presence machine fed the nearest
valid zone cannot tell a person from furniture at the same distance. P01 never could, since it
previously read a single-point source; what changes is that the limitation is now a choice made
in a named file rather than a property of the hardware.

## No threshold lives here

The reduction produces a distance and compares nothing. P01 owns the comparison, through
`range_mm_max` in its settings, and this file has no number of its own to tune. That is deliberate
and it is the reason this is a reduction rather than a detector.

## What it refuses to do, which is the part worth reading

When no zone carries a status the vendor calls a reading, **there is no distance**, and every
obvious way of saying so is wrong in the same direction:

| A tempting return | What P01 would make of it |
|---|---|
| `0` | its test is `range_mm <= range_mm_max`, so zero is the closest possible object. An empty answer becomes a presence |
| `INT16_MAX` | a real surface at 32.7 metres, beyond this part's range, which anything downstream would publish as a measurement |
| a negative value | the part itself returns negatives, ten of them in the noise grid of Saturday 17 October 2026, so this collides with real data |

So the result carries `found` as its own field, and a caller that ignores it gets a distance that
means nothing. The correct response to `found == false` is to post no reading event at all, which
leaves P01's hold to expire on its own timer, a case its table already has rows for.

The result also carries `valid_zones`, because zero valid zones out of sixty-four and a grid that
was never read both produce `found == false` and are different situations. A caller can only tell
them apart if the count comes back.

## The question this sat on top of, and how the data answered it

When this page was written on Sunday 18 October 2026 the only grid this project had recorded, a
coherent surface at about 2.56 metres, reported **status 6 in all sixty-four zones**, and so did
the noise grid from the boot before it. The vendor's header calls 5 and 9 ranging OK, so on the
only real data in existence this reduction returned no reading. Two readings fit and the page
refused to pick between them.

Seven boots on Tuesday 20 October 2026 picked, three of them after the prediction was written:
**status 6 is the first frame after ranging starts, on every boot, and status 5 is every frame
after it.** Both Saturday grids were first frames. A first frame can be a surface or sixty-four
zones of noise and the status does not say which, so the vendor is right to exclude it, and this
reduction handing P01 nothing on frame 1 is the behaviour wanted rather than a defect to work
around.

The same day exercised the refusal on the hardest case. With an object two to ten millimetres from
the sensor face, every zone reported status 4, and the reduction returned no reading eight frames
out of eight. **An object at the face was never turned into a presence at zero millimetres**,
which is the failure the result structure was designed against. And with a hand at thirty
millimetres across half the field, it reported the nearest zone the vendor calls a reading and
passed over every 255, 10, 4 and 13 beside it, including ones holding smaller numbers.

`src/zone_status.h` holds the one definition both the firmware and this reduction use, with the
record of what each observed value accompanied. The valid set is still exactly the vendor's two.

## What proves it

`host/` carries a suite that needs no board, because every answer this reduction can give is
arithmetic:

```
cd projects/05-the-zones/host
make check
make mutants
make selftest
make sanitise
```

`check` runs nine cases whose expected values are written out rather than computed by calling the
thing under test. The two that matter most are refusals: a grid of all status 6 must yield no
reading, and a nearer zone the vendor does not call a reading must not win, which is the entire
purpose of the status field.

`mutants` is the other half. It breaks the reduction four ways and fails unless the suite notices
each:

| The defect | What should catch it |
|---|---|
| the status check stops rejecting | the all status 6 grid, and the nearer invalid zone |
| a tie picks the higher zone index | the tie case, which names the lower index as the rule |
| the first valid zone no longer seeds the comparison | every case with a reading beyond zero millimetres |
| a reading is reported before any zone is examined | the all status 6 grid |

`selftest` proves the guard that protects `mutants` still fires, by reintroducing the defect that
once made that target pass while running nothing. `sanitise` builds the same suite under the
address and undefined-behaviour sanitisers, with `halt_on_error` set, because a warning cannot see
a subscript: this file walks two arrays with one loop variable, narrows that variable for the zone
index, and has a null path, and those are caught at a bad run rather than at a bad value. It
reported no diagnostics on Sunday 18 October 2026.

A suite that has only ever passed is a suite nobody has checked. All four targets run in
continuous integration, so none of them is a thing to remember.

### The harness reported success twice before it ran anything

Worth the paragraph, because this page is asking a reader to trust that table.

- **The first version** looped in the shell over the mutation names and asked for `$SED_$m`. The
  shell has never heard of a make variable, so every expression was empty and every mutation would
  have been a copy of the original. Caught by reading it before it ran.
- **The second version** used a `mutant-%` pattern rule and declared those targets `.PHONY`.
  **GNU make does not search pattern or implicit rules for a phony target.** All four became phony
  targets with no recipe, make did nothing for each, silently, and the summary announced that four
  mutations had been caught. It printed that on the demo laptop on Sunday 18 October 2026, and the
  only tell was four missing lines of output.

The second is the sharper lesson, because it was green and looked green for the right reasons: the
suite passed, the mutations "were caught", and the exit status was zero. **The absence of a failure
was mistaken for the presence of a check**, which is the thing the target exists to prevent, by a
target that had become an example of it.

The third version makes each mutation produce a file and asserts one stamp per mutation, so make's
own machinery is the evidence the work happened. A recipe that does not run leaves no stamp, and
the summary refuses rather than congratulating itself.
