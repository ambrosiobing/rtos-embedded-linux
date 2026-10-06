#!/usr/bin/env python3
"""The three tables are one table: compare them row for row, and fail if they differ.

Project 01 implements the same transition table in C, in C++ and in Rust, and every
page in the project says the three agree row for row. That claim was checked by hand
until this script existed, which is the wrong way round: a claim nobody can re-run is
a claim nobody has checked. This runs in CI over the published sources.

What it compares, per row and in order: the from-state, the event kind, the guard, the
action and the destination. Names are not compared, because each language spells them
differently and a name is not behaviour. Row ORDER is compared, because order is
load-bearing in this table: two rows can share a state and an event and be told apart
only by their guard, so a reordering changes behaviour while leaving every row present.

It also checks the one duplicated fact in the C++ table. Row::guarded is a bool
written out beside the guard pointer, because gcc 13 under the sanitisers will not
treat a function-pointer comparison with nullptr as a constant expression and the
compile-time order check needs a constant. A duplicated fact can drift, so the two are
checked against each other here as well as in the C++ test itself.

Exit status is 0 when the three agree and 1 when they do not, and a disagreement
prints the row index and all three spellings of it.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
P01 = ROOT / "projects" / "01-presence"

# ---------------------------------------------------------------- normalising

STATES = {
    "PRESENCE_FREE": "free",
    "PRESENCE_OCCUPIED": "occupied",
    "PRESENCE_HELD": "held",
    "PRESENCE_FAULT": "fault",
    "State::Free": "free",
    "State::Occupied": "occupied",
    "State::Held": "held",
    "State::Fault": "fault",
}

EVENTS = {
    "PRESENCE_EV_TICK": "tick",
    "PRESENCE_EV_READING": "reading",
    "PRESENCE_EV_TIMEOUT": "timeout",
    "PRESENCE_EV_BUTTON": "button",
    "PRESENCE_EV_FAULT": "fault",
    "PRESENCE_EV_SETTINGS": "settings",
    "Event::Tick": "tick",
    "Event::Reading": "reading",
    "Event::Timeout": "timeout",
    "Event::Button": "button",
    "Event::Fault": "fault",
    "Event::Settings": "settings",
    "Kind::Tick": "tick",
    "Kind::Reading": "reading",
    "Kind::Timeout": "timeout",
    "Kind::Button": "button",
    "Kind::Fault": "fault",
    "Kind::Settings": "settings",
}


def norm_state(raw):
    key = raw.strip()
    if key not in STATES:
        raise SystemExit("crosscheck: unknown state %r" % key)
    return STATES[key]


def norm_event(raw):
    key = raw.strip()
    if key not in EVENTS:
        raise SystemExit("crosscheck: unknown event %r" % key)
    return EVENTS[key]


def norm_fn(raw):
    """A guard or an action, reduced to the bare name the three languages share."""
    name = raw.strip()
    if name in ("NULL", "nullptr", "None"):
        return "none"
    m = re.fullmatch(r"Some\((.+)\)", name)
    if m:
        name = m.group(1).strip()
    name = name.replace("detail::", "")
    if name.startswith("act_"):
        name = name[len("act_") :]
    return name


def block(text, opener, closer, what):
    """The text between an opening line and the first closing line after it."""
    start = text.find(opener)
    if start < 0:
        raise SystemExit("crosscheck: could not find the %s table" % what)
    end = text.find(closer, start)
    if end < 0:
        raise SystemExit("crosscheck: the %s table is not closed" % what)
    return text[start + len(opener) : end]


# ---------------------------------------------------------------- the readers

def read_c():
    text = (P01 / "c" / "presence.c").read_text(encoding="utf-8")
    body = block(text, "static const presence_row_t TABLE[] = {", "\n};", "C")
    rows = []
    # ROW(from, event, guard, action, to), possibly wrapped across lines.
    for m in re.finditer(r"ROW\(([^()]*)\)", body):
        args = [a.strip() for a in m.group(1).split(",")]
        if len(args) != 5:
            raise SystemExit("crosscheck: a C ROW has %d arguments: %r" % (len(args), args))
        rows.append(
            (
                norm_state(args[0]),
                norm_event(args[1]),
                norm_fn(args[2]),
                norm_fn(args[3]),
                norm_state(args[4]),
            )
        )
    return rows


def read_cpp():
    text = (P01 / "cpp" / "presence.hpp").read_text(encoding="utf-8")
    body = block(text, "kTable{{", "\n}};", "C++")
    rows = []
    guarded_flags = []
    for line in body.splitlines():
        line = line.strip()
        if not line.startswith("{State::"):
            continue
        inner = line[1 : line.rindex("}")]
        parts = [a.strip() for a in inner.split(",")]
        if len(parts) != 7:
            raise SystemExit("crosscheck: a C++ row has %d fields: %r" % (len(parts), parts))
        from_, event, guard, guarded, action, to, _name = parts
        rows.append(
            (
                norm_state(from_),
                norm_event(event),
                norm_fn(guard),
                norm_fn(action),
                norm_state(to),
            )
        )
        guarded_flags.append(guarded == "true")
    return rows, guarded_flags


def read_rust():
    text = (P01 / "rust" / "presence_core.rs").read_text(encoding="utf-8")
    body = block(text, "pub static TABLE: [Row; ROW_COUNT] = [", "\n];", "Rust")
    rows = []
    pattern = re.compile(
        r"Row\s*\{\s*from:\s*([^,]+),\s*kind:\s*([^,]+),\s*guard:\s*(None|Some\([^)]*\)),"
        r"\s*action:\s*([^,]+),\s*to:\s*([^,]+),\s*name:"
    )
    for m in pattern.finditer(body):
        rows.append(
            (
                norm_state(m.group(1)),
                norm_event(m.group(2)),
                norm_fn(m.group(3)),
                norm_fn(m.group(4)),
                norm_state(m.group(5)),
            )
        )
    return rows


# ---------------------------------------------------------------------- main

def main():
    c = read_c()
    cpp, guarded_flags = read_cpp()
    rust = read_rust()

    problems = 0

    print("rows parsed: C %d, C++ %d, Rust %d" % (len(c), len(cpp), len(rust)))
    if not (len(c) == len(cpp) == len(rust)):
        print("FAIL the three tables are different lengths")
        return 1
    if len(c) == 0:
        print("FAIL no rows were parsed, which means a parser is broken, not that a table is empty")
        return 1

    # The C++ bool against its own pointer.
    for i, (row, flag) in enumerate(zip(cpp, guarded_flags)):
        has_guard = row[2] != "none"
        if has_guard != flag:
            print("FAIL row %d: the C++ `guarded` bool says %s, the guard is %r"
                  % (i, str(flag).lower(), row[2]))
            problems += 1

    fields = ("from", "event", "guard", "action", "to")
    for i in range(len(c)):
        if c[i] == cpp[i] == rust[i]:
            continue
        print("FAIL row %d differs" % i)
        print("     C    %s" % (dict(zip(fields, c[i])),))
        print("     C++  %s" % (dict(zip(fields, cpp[i])),))
        print("     Rust %s" % (dict(zip(fields, rust[i])),))
        problems += 1

    # Totality, from the C's own rows: every state and event pair has a row.
    pairs = {(row[0], row[1]) for row in c}
    states = sorted({v for k, v in STATES.items() if k.startswith("PRESENCE_")})
    events = sorted({v for k, v in EVENTS.items() if k.startswith("PRESENCE_EV_")})
    for st in states:
        for ev in events:
            if (st, ev) not in pairs:
                print("FAIL the table has no row for (%s, %s), so it is not total" % (st, ev))
                problems += 1

    # An unguarded row must be last among the rows sharing its state and event, or
    # it shadows the guarded ones after it and they can never be taken.
    for i, row in enumerate(c):
        if row[2] != "none":
            continue
        for j in range(i + 1, len(c)):
            if c[j][0] == row[0] and c[j][1] == row[1]:
                print("FAIL row %d is unguarded and row %d shares its state and event, "
                      "so row %d can never be taken" % (i, j, j))
                problems += 1

    if problems:
        print("\nFAILED: %d problem(s)" % problems)
        return 1

    print("the three tables agree on all %d rows, in order, on state, event, guard, "
          "action and destination" % len(c))
    print("the table is total: %d states x %d events, every pair has a row"
          % (len(states), len(events)))
    print("every unguarded row is last among the rows sharing its state and event")
    return 0


if __name__ == "__main__":
    sys.exit(main())
