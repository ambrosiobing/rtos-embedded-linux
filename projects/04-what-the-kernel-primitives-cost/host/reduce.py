#!/usr/bin/env python3
"""Reduce a P04 capture, and refuse the ones that cannot mean anything.

THIS IS THE HALF OF CRITERION 5 THAT CRITERION 6 EXISTS TO PROTECT. Criterion 5 asks that two
instruments agree on the periodic thread's period, within the acquisition board's own
resolution. Criterion 6 asks that a disagreement be detectable. The second is the one that
makes the first worth anything: a reduction which cannot report a disagreement would pass
criterion 5 by being blind, and a check that cannot fail has shown nothing.

So most of this file is refusals, and they are the point rather than defensive clutter.

WHAT A CAPTURE LOOKS LIKE. One self-describing text file, because a run that does not say what
it is becomes unreadable the week after it is taken:

    # anything after a hash is a comment
    instrument    wall
    clock_hz      280000000
    wrap_guard    ok
    resolution_s  0.00001
    a_counts      280011 279998 280004 ...
    b_edges_s     0.000000 0.001000 0.002001 ...

`a_counts` are the processor's own cycle counts for each period, from the instrument that can
see a single cycle. `b_edges_s` are the times the external witness saw an edge, from the
instrument that cannot see anything shorter than its own resolution. The reduction converts the
first to seconds using the stated clock, derives periods from the second, and compares.

THE THREE HEADER FIELDS THAT MOST CAPTURE FORMATS LEAVE OUT, and why all three are required:

  instrument  WHICH on-device counter took the counts, `core` or `wall`. Added Friday 9 October
              2026 after the core cycle counter, the one that prices every other row and that
              Zephyr's own benchmark uses, turned out to stop when the core idles. A period
              brackets a sleep. Across a sleep that counter reads the microseconds the wake-up
              took, not the millisecond that passed, and reports a plausible small number. So
              this reduction, which only ever reduces periods, refuses `core` outright: it is
              not a worse instrument for the job, it is not an instrument for the job.

  clock_hz    A count is not a time. A capture whose clock is absent, zero or unconfirmed is
              refused rather than reduced with a guess, because a guessed rate scales every
              figure by an unknown factor and the result still looks like a measurement.

  wrap_guard  At this processor's clock the cycle counter wraps in about fifteen seconds. A
              bracketed region longer than that yields a small, stable, in-range number, which
              is the worst kind of wrong. The device is required to assert that each region was
              shorter than the wrap; a capture that cannot assert it is refused.

Exit status is 0 when the two instruments agree, and 1 for a disagreement or a refusal. Those
are deliberately the same status: both mean "do not publish a number from this".
"""
from __future__ import annotations

import statistics
import sys
from dataclasses import dataclass, field

# A median of three samples is not a median. The floor is low enough to keep a short bench run
# usable and high enough that a run of four cannot produce a published figure.
MIN_SAMPLES = 20


class Refusal(Exception):
    """A capture that cannot mean anything, with the reason a reader needs."""


@dataclass
class Capture:
    instrument: str = ""
    clock_hz: int = 0
    wrap_guard: str = ""
    resolution_s: float = 0.0
    a_counts: list[int] = field(default_factory=list)
    b_edges_s: list[float] = field(default_factory=list)


def parse(text: str) -> Capture:
    """Read a capture, raising Refusal rather than guessing at anything missing."""
    cap = Capture()
    seen: set[str] = set()

    for lineno, raw in enumerate(text.splitlines(), start=1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue

        key, _, rest = line.partition(" ")
        values = rest.split()
        if not values:
            raise Refusal(f"line {lineno}: key {key!r} has no value")
        if key in seen:
            raise Refusal(f"line {lineno}: key {key!r} appears twice, so which one is the run?")
        seen.add(key)

        try:
            if key == "instrument":
                cap.instrument = values[0]
            elif key == "clock_hz":
                cap.clock_hz = int(values[0])
            elif key == "wrap_guard":
                cap.wrap_guard = values[0]
            elif key == "resolution_s":
                cap.resolution_s = float(values[0])
            elif key == "a_counts":
                cap.a_counts = [int(v) for v in values]
            elif key == "b_edges_s":
                cap.b_edges_s = [float(v) for v in values]
            else:
                raise Refusal(f"line {lineno}: unknown key {key!r}")
        except ValueError as exc:
            raise Refusal(f"line {lineno}: {key} does not parse, {exc}") from exc

    return cap


def validate(cap: Capture) -> None:
    """Every reason a capture cannot produce a number, checked before it produces one."""
    if cap.instrument == "":
        raise Refusal(
            "instrument is absent. A period measured by a counter that stops when the core "
            "idles reads as a plausible small number, so the capture has to say which counter "
            "took it"
        )
    if cap.instrument == "core":
        raise Refusal(
            "instrument is 'core', which stops when the core idles, and a period brackets a "
            "sleep. That counter is not an instrument for this job: across a sleep it reads the "
            "wake-up and not the interval"
        )
    if cap.instrument != "wall":
        raise Refusal(f"instrument is {cap.instrument!r}, and the only one that can bracket a sleep is 'wall'")
    if cap.clock_hz <= 0:
        raise Refusal(
            "clock_hz is absent or zero. A count is not a time, and a guessed rate scales "
            "every figure by an unknown factor while still looking like a measurement"
        )
    if cap.wrap_guard != "ok":
        # The device names which fault it hit, so the refusal can too. Before Friday 9 October
        # 2026 every fault came back as 'unknown' and a reader of a zero-count capture was sent
        # looking for a wrap.
        why = {
            "wrapped": (
                "a count passed the wrap limit. The counter wraps in about fifteen seconds at "
                "this clock, and a wrapped region reads as a plausible small number rather "
                "than as an error"
            ),
            "zero": (
                "a count was zero, which is a region that was never bracketed or one bracketed "
                "around nothing, and either way is not a measurement"
            ),
            "unknown": "the device vouches for nothing, which includes a run with no samples",
        }.get(cap.wrap_guard, "the device did not vouch for these counts")
        raise Refusal(f"wrap_guard is {cap.wrap_guard!r} rather than 'ok': {why}")
    if cap.resolution_s <= 0.0:
        raise Refusal(
            "resolution_s is absent or zero. Agreement is meaningless without the tolerance "
            "the witness is actually capable of"
        )
    if len(cap.a_counts) < MIN_SAMPLES:
        raise Refusal(
            f"only {len(cap.a_counts)} cycle counts, and {MIN_SAMPLES} is the floor. "
            "A median of a handful is not a median"
        )
    if len(cap.b_edges_s) < MIN_SAMPLES + 1:
        raise Refusal(
            f"only {len(cap.b_edges_s)} witness edges, which yields "
            f"{max(len(cap.b_edges_s) - 1, 0)} periods, and {MIN_SAMPLES} is the floor"
        )
    if any(c <= 0 for c in cap.a_counts):
        raise Refusal("a cycle count is zero or negative, which no elapsed region produces")
    if any(b <= a for a, b in zip(cap.b_edges_s, cap.b_edges_s[1:])):
        raise Refusal("witness edge times are not increasing, so the capture is out of order")


def periods_from_counts(cap: Capture) -> list[float]:
    return [c / cap.clock_hz for c in cap.a_counts]


def periods_from_edges(cap: Capture) -> list[float]:
    return [b - a for a, b in zip(cap.b_edges_s, cap.b_edges_s[1:])]


def summarise(values: list[float]) -> dict[str, float]:
    return {
        "min": min(values),
        "median": statistics.median(values),
        "max": max(values),
    }


def reduce_capture(text: str) -> tuple[bool, str]:
    """Return whether the two instruments agree, and the text a reader should see.

    The agreement is between MEDIANS, not means. A mean is moved by one late sample and the
    number a design has to survive disappears into it; the median says what the period usually
    is and the maximum is reported beside it so the outlier is visible rather than averaged
    away.
    """
    cap = parse(text)
    validate(cap)

    a = summarise(periods_from_counts(cap))
    b = summarise(periods_from_edges(cap))
    difference = abs(a["median"] - b["median"])
    agree = difference <= cap.resolution_s

    lines = [
        f"instrument       {cap.instrument}, from the capture",
        f"clock            {cap.clock_hz} Hz, from the capture",
        f"witness can see  {cap.resolution_s * 1e6:.1f} us, from the capture",
        f"samples          {len(cap.a_counts)} counts, {len(cap.b_edges_s) - 1} witness periods",
        "",
        "                      min          median         max",
        f"  cycle counter  {a['min'] * 1e6:9.3f} us  {a['median'] * 1e6:9.3f} us  "
        f"{a['max'] * 1e6:9.3f} us",
        f"  witness        {b['min'] * 1e6:9.3f} us  {b['median'] * 1e6:9.3f} us  "
        f"{b['max'] * 1e6:9.3f} us",
        "",
        f"medians differ by {difference * 1e6:.3f} us against a tolerance of "
        f"{cap.resolution_s * 1e6:.1f} us",
    ]

    if agree:
        lines.append("AGREE the two instruments report the same period")
    else:
        lines.append(
            "DISAGREE the two instruments report different periods. That points at the clock "
            "configuration rather than at the kernel, because both are watching one thread"
        )

    return agree, "\n".join(lines)


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print("usage: reduce.py <capture file>", file=sys.stderr)
        return 2

    with open(argv[0], encoding="utf-8") as handle:
        text = handle.read()

    try:
        agree, report = reduce_capture(text)
    except Refusal as exc:
        print(f"REFUSED {exc}")
        return 1

    print(report)
    return 0 if agree else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
