#!/usr/bin/env python3
"""The C emits a capture, the Python parses one, and this is where they have to agree.

TWO DESCRIPTIONS OF ONE FORMAT IN TWO LANGUAGES is two chances to write a different format, and
no test inside either can notice the drift, because each is right about itself. P01 met that
shape with one table in three languages and P02 with one cascade in three. This is the same
problem wearing a capture file.

So nothing here is hand-written data. Every capture under test comes out of the compiled core,
and the host adds only the two fields the device genuinely cannot know: what the external
witness resolves, and when it saw an edge.

Run it as `test_capture_pipeline.py <path to emit_capture>`, or let the Makefile do it.
"""
from __future__ import annotations

import subprocess
import sys

from reduce import Refusal, reduce_capture

PERIOD_S = 0.001
RESOLUTION_S = 0.00001
EDGES = 41

# THE ONLY OPERATION THE REDUCTION REDUCES. Index 6 is the periodic thread period, the one row
# bracketing a sleep, the one row the witness can see, and the one row the emitter labels with
# the `wall` instrument. Every whole-capture case below uses it, because since Friday 9 October
# 2026 a capture from any other row is refused for its instrument before anything else about it
# is looked at, and a refusal test that refuses for a different reason than it names has not
# tested what it names.
PERIOD_OP = 6
CORE_OPS = range(6)


def witness_half(period_s: float = PERIOD_S) -> str:
    """The two fields the device cannot know, which the host adds from the acquisition board."""
    edges = " ".join(f"{i * period_s:.9f}" for i in range(EDGES))
    return f"resolution_s {RESOLUTION_S}\nb_edges_s {edges}\n"


def device_half(binary: str, op: int, mode: str) -> str:
    run = subprocess.run([binary, str(op), mode], capture_output=True, text=True, check=True)
    return run.stdout


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print("usage: test_capture_pipeline.py <path to emit_capture>", file=sys.stderr)
        return 2
    binary = argv[0]
    ok = True

    # THE HALF ON ITS OWN MUST BE REFUSED. A device half fed to the reduction by mistake is
    # missing the witness fields entirely, and being refused is the behaviour we want from an
    # accident rather than a reduction that quietly assumes a tolerance.
    try:
        reduce_capture(device_half(binary, PERIOD_OP, "clean"))
        print("FAIL a device half alone was reduced, and it should have been refused")
        ok = False
    except Refusal:
        pass

    # The whole capture, assembled the way a bench evening would assemble it.
    whole = device_half(binary, PERIOD_OP, "clean") + witness_half()
    try:
        agree, _ = reduce_capture(whole)
        if not agree:
            print("FAIL a clean capture from the core was reduced as a disagreement")
            ok = False
    except Refusal as exc:
        print(f"FAIL a clean capture from the core was refused: {exc}")
        ok = False

    # EVERY OPERATION NAME SURVIVES THE ROUND TRIP. The core writes the name into a comment and
    # docs/RESULTS.md carries the same string in its Operation column, so a rename in one place
    # without the other is caught here rather than by a reader.
    for op in range(7):
        text = device_half(binary, op, "clean")
        if not text.startswith("# p04 device half, "):
            print(f"FAIL op {op}: the capture does not name its operation")
            ok = False

    # THE EMITTER LABELS EACH ROW WITH ITS INSTRUMENT, AND THE LABEL IS THE ONE THE CORE'S TABLE
    # SAYS. Six rows bracket no idle and say `core`; the period brackets a sleep and says `wall`.
    # Checked row by row rather than by one example, because the table in measure.c is the thing
    # under test and a table is wrong one entry at a time.
    for op in CORE_OPS:
        if "\ninstrument core\n" not in device_half(binary, op, "clean"):
            print(f"FAIL op {op}: a bracket with no sleep in it is not labelled 'core'")
            ok = False
    if "\ninstrument wall\n" not in device_half(binary, PERIOD_OP, "clean"):
        print("FAIL the period, the one bracket containing a sleep, is not labelled 'wall'")
        ok = False

    # AND THE PARSER INSISTS ON IT. A core-instrument capture with a perfectly good witness half
    # attached is refused, for the instrument and for nothing else. The reason is matched, not
    # only the refusal: refusing for some other reason would mean the instrument check never ran.
    try:
        reduce_capture(device_half(binary, 0, "clean") + witness_half())
        print("FAIL a 'core' capture was reduced as a period, and that counter stops in a sleep")
        ok = False
    except Refusal as exc:
        if "instrument is 'core'" not in str(exc):
            print(f"FAIL a 'core' capture was refused, but not for its instrument: {exc}")
            ok = False

    # THE REFUSALS HAVE TO TRAVEL IN THE CAPTURE. Each of these is a condition the device knows
    # about and the host cannot reconstruct, so the capture has to carry it rather than leaving
    # it to whoever reads the file. The reason is matched for the same cause as above: these
    # three used to run on op 0, which the instrument check now refuses first, and all three
    # would have kept passing while testing nothing they name.
    for mode, why, reason in (
        ("wrapped", "a count past the wrap limit", "wrap_guard"),
        ("zerocount", "a zero elapsed count", "zero or negative"),
        ("zeroclock", "an unconfirmed clock tree", "clock_hz"),
    ):
        text = device_half(binary, PERIOD_OP, mode) + witness_half()
        try:
            reduce_capture(text)
            print(f"FAIL {mode}: {why} was reduced instead of being refused")
            ok = False
        except Refusal as exc:
            if reason not in str(exc):
                print(f"FAIL {mode}: refused, but not for {why}: {exc}")
                ok = False

    # And the loop closes the other way: a capture the core calls sound, with a period the
    # witness disagrees about, is reported as a disagreement rather than refused.
    disagreeing = device_half(binary, PERIOD_OP, "clean") + witness_half(PERIOD_S + 0.00005)
    try:
        agree, _ = reduce_capture(disagreeing)
        if agree:
            print("FAIL a 50 us difference between the two instruments was not reported")
            ok = False
    except Refusal as exc:
        print(f"FAIL a disagreement was refused rather than reported: {exc}")
        ok = False

    print("the emitter and the parser agree" if ok else "the emitter and the parser do not agree")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
