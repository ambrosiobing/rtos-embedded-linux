#!/usr/bin/env python3
"""The WITNESS HALF of a P04 capture, from the acquisition board's own recording.

    python3 witness.py run-007.csv run-007.json > witness.txt
    cat device.txt witness.txt > whole.txt
    python3 reduce.py whole.txt

The device half says what the processor's counter saw. This says what the MCC 118 saw, which
is the only other instrument on the bench, and it is the half the device genuinely cannot
produce: it does not know what the witness resolves or when the witness saw an edge.

WHAT IT READS. The two files that `scan.py` in the sibling firmware volume writes: a CSV of one
voltage per line, and a JSON of metadata carrying `actual_rate_hz`, `overrun` and `samples`.
Reading that format here rather than importing that script keeps the two volumes separate at
the cost of one duplicated edge finder, and the cost is paid deliberately: the duplicate is
tested against a synthetic recording whose edges are known by construction, below in
`test_witness.py`, so a drift between the two would be found by a failing test rather than by
a disagreement on a bench evening.

WHAT THE MARKER LOOKS LIKE. One edge per period, alternating, never a pulse. The firmware
toggles the pin at each period boundary, so every crossing in either direction is a boundary
and the interval between consecutive edges is one period. A pulse was tried first in the
sibling volume and the 10 microsecond sampler caught fragments of it; see its marker.c.

THE REFUSALS ARE THE POINT. A flat recording is not "no edges", it is a wire not fitted or a
pin not driven, and this project has had the first of those for most of a day. An overrun is a
recording with a gap in it. Each refusal names what a reader should go and check.

Exit status is 0 when a witness half was written, 1 otherwise.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

# Fewer than this and there is nothing to find a threshold in, let alone an edge.
MIN_SAMPLES = 16

# A marker swing smaller than this is noise, not a pin. The MCC 118's resting floor is about two
# converter codes wide and the marker drives 3.3 V, so a tenth of a volt separates the two
# cases by more than an order of magnitude either way.
MIN_SWING_V = 0.1


class Refusal(Exception):
    """A recording that cannot yield edges, with the reason a reader can act on."""


def load_samples(csv_path: Path) -> list[float]:
    """One voltage per line. Lines that do not start like a number are skipped as headers."""
    out: list[float] = []
    for line in csv_path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line[0] not in "-+.0123456789":
            continue
        out.append(float(line.split(",")[0]))
    return out


def load_meta(json_path: Path) -> dict:
    """The metadata as written. What it must contain is checked where it is used, in
    witness_half(), so that a caller handing in a dict of its own gets the same refusals as one
    reading a file; the first version checked here and witness_half() then fell over on a
    missing key when the test bypassed the loader."""
    return json.loads(json_path.read_text(encoding="utf-8"))


def marker_edges(samples: list[float], fs: float) -> list[float]:
    """Edge times in seconds, rising and falling alike.

    Two thresholds at 30 and 70 per cent of the observed swing, so a recording with an offset
    or through an attenuator still works, and the pair is also the hysteresis: a rise counts
    only from an established low and a fall only from an established high. The crossing of the
    midpoint is interpolated between the two samples around it, which is what puts an edge
    time below one sample period.

    This is the method of rate.py in the sibling firmware volume, written out here rather than
    imported, and tested against a synthetic recording in test_witness.py.
    """
    if len(samples) < MIN_SAMPLES:
        raise Refusal(f"only {len(samples)} samples, and {MIN_SAMPLES} is the floor")

    lo, hi = min(samples), max(samples)
    swing = hi - lo
    if swing < MIN_SWING_V:
        raise Refusal(
            f"the recording is flat, a swing of {swing * 1000:.1f} mV. That is a wire not fitted "
            "or a pin not driven, and the capture has recorded the absence of a marker rather "
            "than a marker"
        )

    low = lo + 0.3 * swing
    high = lo + 0.7 * swing
    mid = 0.5 * (low + high)

    edges: list[float] = []
    is_high = samples[0] >= mid
    for i in range(1, len(samples)):
        prev, cur = samples[i - 1], samples[i]
        rise = (not is_high) and cur >= high
        fall = is_high and cur < low
        if not rise and not fall:
            continue
        if cur != prev:
            frac = (mid - prev) / (cur - prev)
            frac = min(max(frac, 0.0), 1.0)
        else:
            frac = 0.0
        edges.append((i - 1 + frac) / fs)
        is_high = rise
    return edges


def witness_half(samples: list[float], meta: dict, source: str) -> str:
    """The two lines reduce.py needs from the witness, plus a comment saying where they came from."""
    if meta.get("overrun"):
        raise Refusal(
            "the HAT reported an overrun, so this recording has a gap in it and an interval "
            "across the gap would read as a period that never happened"
        )
    if "actual_rate_hz" not in meta:
        raise Refusal(
            "the metadata has no actual_rate_hz, so the edge times would be in samples and not "
            "in seconds, and a count is not a time"
        )
    fs = float(meta["actual_rate_hz"])
    if fs <= 0:
        raise Refusal(f"actual_rate_hz is {fs}, which is not a sample rate")

    edges = marker_edges(samples, fs)
    if len(edges) < 2:
        raise Refusal(
            f"{len(edges)} edge in {len(samples) / fs:.3f} s of recording. The pin moved at "
            "most once, which is a reset and not a period"
        )

    lines = [
        f"# p04 witness half, from {source}, {len(samples)} samples at {fs:.3f} Hz,"
        f" {len(edges)} edges both polarities",
        f"resolution_s {1.0 / fs:.9f}",
        "b_edges_s " + " ".join(f"{e:.9f}" for e in edges),
    ]
    return "\n".join(lines) + "\n"


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: witness.py <capture.csv> <capture.json>", file=sys.stderr)
        return 2
    csv_path, json_path = Path(argv[0]), Path(argv[1])
    # A missing file is the commonest way this is run wrong, and on Friday 9 October 2026 it
    # produced a traceback because the capture had been copied to the other laptop. A refusal
    # that names the path is what a person at a shell needs; a traceback names pathlib.
    for p in (csv_path, json_path):
        if not p.is_file():
            print(f"REFUSED no such file: {p}. The capture may be on the other machine", file=sys.stderr)
            return 1
    try:
        samples = load_samples(csv_path)
        meta = load_meta(json_path)
        sys.stdout.write(witness_half(samples, meta, csv_path.name))
    except Refusal as exc:
        print(f"REFUSED {exc}", file=sys.stderr)
        return 1
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"REFUSED {csv_path.name} or {json_path.name} could not be read: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
