# P04. What the kernel primitives cost, by the cycle counter

Status on Thursday 8 October 2026: **the design page exists and no code does.** No number
here has been measured and every row of the eventual table reads `not measured`.

[docs/DESIGN.md](docs/DESIGN.md) sorts the chapter's seven acceptance criteria by what each
costs, and **two of them need no hardware at all**: that the reduction can report a
disagreement it was shown, and that no row of the table cites an instrument which cannot see
what the row claims. Those two come first, because a check that cannot fail has shown nothing.

It also records the one decision that goes beyond the chapter, with its cost: the same
primitives will be priced under **two kernels**, so that a ratio between primitives can be
shown to survive a change of kernel rather than being a property of one.

The written specification is [chapter 04](../../chapters/04-what-the-kernel-primitives-cost.md),
which is complete. What is absent is the code.

| | |
|---|---|
| Target | NUCLEO-H7A3ZI-Q, with an MCC 118 on a Raspberry Pi as a witness for periods only |
| Board | NUCLEO-H7A3ZI-Q, with a Raspberry Pi carrying an MCC 118 as an external witness |
| Peripherals | The data watchpoint unit's cycle counter, one general-purpose pin for the witness, the virtual console |
| Toolchain | As the front matter, plus a short host script that reduces the captured periods |
| Operating system | Several threads, a message queue, a work queue, a mutex |
| Difficulty | 4 of 5 |
| Effort | 4 evenings of about four hours |

## What exists, and what it was shown to catch

[host/reduce.py](host/reduce.py) reduces a capture and, mostly, refuses it. The capture format
carries two fields most leave out, and both come from this volume's own scars: **`clock_hz`**,
because a count is not a time and a guessed rate scales every figure by an unknown factor while
still looking like a measurement, and **`wrap_guard`**, because the cycle counter wraps in about
fifteen seconds at this clock and a wrapped region reads as a plausible small number rather
than as an error.

[host/test_reduce.py](host/test_reduce.py) is three accept cases, three disagreement cases and
eleven refusals. **The suite was shown to go red before it was committed**, which is the only
thing that makes a passing suite worth reading:

| Mutation applied to `reduce.py` | Caught by |
|---|---|
| the clock rate refusal removed | `no clock rate`, and `a clock rate of zero` |
| the wrap guard refusal removed | `the wrap guard is unknown`, and `the wrap guard failed` |
| the sample floor removed | `too few cycle counts, with plenty of witness edges` |
| the tolerance widened a hundredfold | all three disagreement cases |
| agreement hard-coded true | all three disagreement cases |
| the median replaced by the mean | `one late sample does not move the median` |

**Two of those six survived the first version of the suite**, and fixing the suite rather than
the record is the reason the table is here.

The sample floor mutation was masked by a second, redundant floor: the case was short on both
instruments at once, so removing the one under test left the other to refuse the capture
anyway. **A redundant check hides a missing test.** The case now runs short on one instrument
and long on the other.

The median mutation survived because every sample in the data was near-uniform, so the mean and
the median agreed. The design chooses the median precisely because a mean is moved by one late
sample, and nothing enforced that choice. There is now a case with thirty-nine nominal periods
and one that took ten times as long, where a mean would be dragged twenty-two times the
tolerance away.

A third thing came out of the run. Removing the clock-rate refusal let a zero clock reach a
division, so the suite ended in a traceback rather than a named failure. It was red, which is
what the mutation asked for, and it named nothing. Every case now reports an unexpected
exception as a failure of that case, because the matrix above is read by a person.

## What it would deliver

A table of primitive costs in cycles, each with its own method and its own refutation, and a witness that confirms the one claim the cycle counter cannot make about itself

## Figures

The chapter's five figures are in [docs/figures](docs/figures), as rendered
SVG. The LaTeX that draws them stays on the authoring machine: this repository
publishes no `.tex`, and `checks.yml` fails if one is ever committed.
