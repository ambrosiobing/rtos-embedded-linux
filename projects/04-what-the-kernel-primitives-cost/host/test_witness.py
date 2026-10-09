#!/usr/bin/env python3
"""witness.py against recordings whose edges are known by construction.

The edge finder is a duplicate of one in the sibling firmware volume, duplicated on purpose so
the two volumes stay separate. A duplicate is a drift waiting to happen, so this is where it is
held to the thing it claims: a synthetic square wave at a known period, with the MCC 118's own
kind of noise on it, must come back as that period to within the witness's resolution.

Nothing here is a recording from the bench. A bench recording tests the bench; this tests the
arithmetic, and the two have to be tested separately or a failure cannot be attributed.

Run with no arguments. Exit status 0 when every case behaves, 1 otherwise.
"""
from __future__ import annotations

import json
import random
import statistics
import sys

from reduce import Refusal as ReduceRefusal, reduce_capture
from witness import MIN_SAMPLES, Refusal, marker_edges, witness_half

FS = 100_000.0       # the MCC 118 on one channel
PERIOD_S = 0.0011    # 1.1 ms, which is what the board actually produces at this tick
EDGES = 64           # one toggle per period, so one edge per period
SWING_V = 3.3


def square_wave(period_s: float, edges: int, fs: float, noise_v: float = 0.004,
                offset_v: float = 0.0, lead_s: float = 0.05, seed: int = 1) -> list[float]:
    """A marker that toggles every `period_s`, recorded at `fs`, flat before and after.

    Noise is two converter codes or so, which is the floor the bench reported. The flat lead-in
    and lead-out matter: the firmware only toggles during its 64 periods and the recording is
    seconds long, so most of a real capture is the pin sitting still.
    """
    rng = random.Random(seed)
    total_s = lead_s * 2 + period_s * (edges + 1)
    n = int(total_s * fs)
    out: list[float] = []
    for i in range(n):
        t = i / fs
        # Segment k begins at edge k. The firmware toggles at edge 0 and at each of the
        # `edges` boundaries after it, so segments 0 to `edges` alternate starting high, and
        # after the last toggle the level stays where it was left. The first version of this
        # generator produced one edge too few and the test caught its own fixture, which is
        # what a fixture built from arithmetic rather than from a recording is for.
        k = int((t - lead_s) / period_s) if t >= lead_s else -1
        high = k >= 0 and (min(k, edges) % 2 == 0)
        level = SWING_V if high else 0.0
        out.append(level + offset_v + rng.gauss(0.0, noise_v))
    return out


def meta(fs: float = FS, overrun: bool = False) -> dict:
    return {"actual_rate_hz": fs, "overrun": overrun, "samples": 0}


def check(name: str, ok: bool, detail: str = "") -> bool:
    if not ok:
        print(f"FAIL {name}" + (f": {detail}" if detail else ""))
    return ok


def main() -> int:
    ok = True

    # THE ONE ACCEPT CASE THAT MATTERS. A 1.1 ms square wave comes back as 1.1 ms, to within
    # the resolution the witness actually has, from edges found both polarities.
    samples = square_wave(PERIOD_S, EDGES, FS)
    edges = marker_edges(samples, FS)
    periods = [b - a for a, b in zip(edges, edges[1:])]
    ok &= check("edge count", len(edges) == EDGES + 1, f"found {len(edges)}, wanted {EDGES + 1}")
    med = statistics.median(periods) if periods else float("nan")
    ok &= check("median period within one sample of 1.1 ms",
                abs(med - PERIOD_S) <= 1.0 / FS, f"median {med * 1e6:.2f} us")
    ok &= check("worst period within two samples of 1.1 ms",
                all(abs(p - PERIOD_S) <= 2.0 / FS for p in periods),
                f"worst {max(abs(p - PERIOD_S) for p in periods) * 1e6:.2f} us" if periods else "")

    # An offset recording, as through an attenuator or a floating ground, still yields edges,
    # because the thresholds come from the observed swing and not from 0 and 3.3.
    shifted = square_wave(PERIOD_S, EDGES, FS, offset_v=0.8)
    ok &= check("edges survive an offset", len(marker_edges(shifted, FS)) == EDGES + 1)

    # THE WHOLE LOOP, witness half into reduce.py beside a matching device half. This is the
    # closure criterion 5 needs: two instruments, one capture, agreement reported.
    counts = [int(round(PERIOD_S * 280_000_000))] * EDGES
    device = ("instrument wall\nclock_hz 280000000\nwrap_guard ok\n"
              "a_counts " + " ".join(str(c) for c in counts) + "\n")
    try:
        agree, _ = reduce_capture(device + witness_half(samples, meta(), "synthetic"))
        ok &= check("the loop closes in agreement", agree)
    except ReduceRefusal as exc:
        ok &= check("the loop closes in agreement", False, f"refused: {exc}")

    # And criterion 6 through the whole loop: a device half claiming 1.0 ms against a witness
    # that saw 1.1 ms is reported as a disagreement, not refused and not agreed.
    wrong = ("instrument wall\nclock_hz 280000000\nwrap_guard ok\n"
             "a_counts " + " ".join(str(280_000) for _ in range(EDGES)) + "\n")
    try:
        agree, _ = reduce_capture(wrong + witness_half(samples, meta(), "synthetic"))
        ok &= check("a 100 us disagreement is reported through the loop", not agree)
    except ReduceRefusal as exc:
        ok &= check("a 100 us disagreement is reported through the loop", False, f"refused: {exc}")

    # THE REFUSALS. Each is a recording that looks like data and is not.
    def must_refuse(name: str, fn) -> bool:
        try:
            fn()
        except Refusal:
            return True
        except Exception as exc:  # noqa: BLE001
            print(f"FAIL {name}: expected a refusal, got {exc!r}")
            return False
        print(f"FAIL {name}: expected a refusal, got a result")
        return False

    flat = [0.0 + random.Random(2).gauss(0, 0.004) for _ in range(20_000)]
    ok &= must_refuse("a flat recording, the wire-not-fitted case", lambda: marker_edges(flat, FS))
    ok &= must_refuse("too few samples", lambda: marker_edges(samples[:MIN_SAMPLES - 1], FS))
    ok &= must_refuse("an overrun", lambda: witness_half(samples, meta(overrun=True), "x"))
    ok &= must_refuse("no sample rate", lambda: witness_half(samples, {"overrun": False}, "x"))
    one_edge = square_wave(PERIOD_S, 0, FS)
    ok &= must_refuse("a single edge, which is a reset and not a period",
                      lambda: witness_half(one_edge, meta(), "x"))

    print("every case behaved" if ok else "at least one case did not behave")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
