#!/usr/bin/env python3
"""P04 criterion 7: no row may claim an instrument that cannot see what the row claims.

The acquisition board on this bench resolves about ten microseconds on one channel. A context
switch is far shorter than that. **Measuring one with the other would produce a number, and the
number would be a property of the instrument rather than of the software**, which is the kind of
figure that is impossible to spot later because it is plausible, stable and in range.

So the rule is enforced rather than remembered. It is the rule most likely to be broken quietly
by somebody adding a row in a hurry, months after the sentence explaining it was written.

Four things are checked, and only the first is criterion 7 itself. The other three exist because
a table that can be relabelled to suit a number is not a check:

  1. a row predicted to be below the witness's resolution must name the cycle counter
  2. a measured duration must agree with its own Scale column, so a row cannot be quietly
     relabelled when a number comes out the other side of the boundary
  3. a cycle count and a duration appear together or not at all, because a count is not a time
     until a rate is stated
  4. every below-resolution operation has both a warm and a cold row, because reporting one of
     them alone reports an accident of ordering

Run with no arguments. Exit status 0 when the table is sound, 1 otherwise, naming every row that
is not.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLE = ROOT / "projects" / "04-what-the-kernel-primitives-cost" / "docs" / "RESULTS.md"

# From chapter 04: the acquisition board resolves about ten microseconds on one channel. This is
# the one number the whole check turns on, so it is stated once, here, with where it came from.
WITNESS_RESOLUTION_US = 10.0

CYCLE_COUNTER = "cycle counter"
WITNESS = "witness"
INSTRUMENTS = {CYCLE_COUNTER, WITNESS}
SCALES = {"below", "above"}
CACHES = {"warm", "cold", "n/a"}
UNMEASURED = "not measured"

COLUMNS = ["Operation", "Kernel", "Scale", "Instrument", "Cache", "Cycles", "Duration us"]


def parse_rows(text: str) -> list[dict[str, str]]:
    """Pull the one pipe table out of the page, and refuse a page whose header has drifted."""
    rows: list[dict[str, str]] = []
    header_seen = False

    for raw in text.splitlines():
        line = raw.strip()
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]

        if not header_seen:
            if cells != COLUMNS:
                raise SystemExit(
                    f"the table header is {cells}, and this check was written against "
                    f"{COLUMNS}. Reconcile them rather than loosening this check"
                )
            header_seen = True
            continue
        if all(set(c) <= set("-: ") for c in cells):
            continue
        if len(cells) != len(COLUMNS):
            raise SystemExit(f"a row has {len(cells)} cells against {len(COLUMNS)} columns: {line}")
        rows.append(dict(zip(COLUMNS, cells)))

    if not header_seen:
        raise SystemExit("no table found, so nothing was checked, which is worse than a failure")
    return rows


def as_number(value: str) -> float | None:
    if value == UNMEASURED:
        return None
    try:
        return float(value.split()[0])
    except (ValueError, IndexError):
        raise SystemExit(f"{value!r} is neither {UNMEASURED!r} nor a number") from None


def check(rows: list[dict[str, str]]) -> list[str]:
    problems: list[str] = []
    warm_cold: dict[tuple[str, str], set[str]] = {}

    for row in rows:
        where = f"{row['Operation']} [{row['Kernel']}, {row['Cache']}]"

        if row["Instrument"] not in INSTRUMENTS:
            problems.append(f"{where}: instrument {row['Instrument']!r} is not one this bench has")
            continue
        if row["Scale"] not in SCALES:
            problems.append(f"{where}: scale {row['Scale']!r} is not 'below' or 'above'")
            continue
        if row["Cache"] not in CACHES:
            problems.append(f"{where}: cache {row['Cache']!r} is not 'warm', 'cold' or 'n/a'")
            continue

        # 1. Criterion 7 itself.
        if row["Scale"] == "below" and row["Instrument"] != CYCLE_COUNTER:
            problems.append(
                f"{where}: predicted below {WITNESS_RESOLUTION_US} us and measured with the "
                f"{row['Instrument']}, which cannot see it. That number would be a property "
                "of the instrument"
            )

        cycles = as_number(row["Cycles"])
        duration = as_number(row["Duration us"])

        # 3. A count is not a time.
        if (cycles is None) != (duration is None):
            problems.append(
                f"{where}: cycles and duration must appear together. A count without a "
                "duration is not reportable, because it is not a time until a rate is stated"
            )

        # 2. The prediction has to survive the measurement.
        if duration is not None:
            if row["Scale"] == "below" and duration >= WITNESS_RESOLUTION_US:
                problems.append(
                    f"{where}: predicted below {WITNESS_RESOLUTION_US} us and measured "
                    f"{duration} us. Change the measurement or the prediction, and say which"
                )
            if row["Scale"] == "above" and duration < WITNESS_RESOLUTION_US:
                problems.append(
                    f"{where}: predicted above {WITNESS_RESOLUTION_US} us and measured "
                    f"{duration} us, which the witness could not have seen"
                )

        if row["Scale"] == "below":
            warm_cold.setdefault((row["Operation"], row["Kernel"]), set()).add(row["Cache"])
        elif row["Cache"] != "n/a":
            problems.append(f"{where}: an above-resolution row states a cache, which it has no use for")

    # 4. Neither half of the warm and cold pair is reported alone.
    for (operation, kernel), caches in sorted(warm_cold.items()):
        if caches != {"warm", "cold"}:
            problems.append(
                f"{operation} [{kernel}]: has {sorted(caches)} and needs both warm and cold. "
                "One of them alone reports an accident of ordering"
            )

    return problems


def main() -> int:
    rows = parse_rows(TABLE.read_text(encoding="utf-8"))
    problems = check(rows)

    for problem in problems:
        print(f"  {problem}")

    measured = sum(1 for r in rows if r["Duration us"] != UNMEASURED)
    print(
        f"{len(rows)} rows checked, {measured} measured, {len(problems)} problem(s)"
    )
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
