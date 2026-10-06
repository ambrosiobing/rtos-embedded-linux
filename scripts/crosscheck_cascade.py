#!/usr/bin/env python3
"""P02's cascade in C, C++ and Rust, compared arm for arm.

    python scripts/crosscheck_cascade.py

WHY THIS EXISTS. Three implementations of one policy are three chances to write a different
policy. Each language's own test checks that ITS cascade behaves, and no test in any of them
can notice that the three have drifted apart, because each is right about itself. This reads
the three sources and compares them.

WHAT IT COMPARES, per arm and in order: the guard's name and the code it produces. The guard
names are deliberately identical across the three languages, which is the whole reason
claim.c names its predicates instead of writing the conditions inline: the three spell the
same test differently, so `in->window_open && in->present` and `in.window_open && in.present`
are the same policy and nothing can say so, while `booked_and_present` and
`booked_and_present` are comparable by a script.

The three write the cascade in different shapes on purpose, and the shapes are not the
claim. C uses a chain of ifs, C++ a constexpr array of {guard, code}, Rust a static array of
{Option<fn>, code}. Each is the form its language reasons about best. What must agree is the
ordered list of (guard, code) pairs, and that is what this extracts from each.

IT ALSO CHECKS THE THREE THINGS THE ORDER HAS TO SATISFY: exactly one arm is unguarded, it
is the last, and every code the policy declares is produced by some arm.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# The project root is an argument so that the mutation runs which prove this check can fail
# can operate on a copy. A checker whose scope is fixed in the source cannot be tested
# without editing the thing it is meant to be guarding, and one that is never tested is a
# zero nobody has earned. scripts/check_string_literals.py learned that the expensive way.
P02 = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "projects" / "02-the-claim"

ARMS_EXPECTED = 8


def canon_code(raw):
    """A code name reduced to the spelling the three languages share."""
    name = raw.strip()
    name = name.replace("Code::", "")
    if name.startswith("CLAIM_"):
        name = name[len("CLAIM_") :]
        return name.lower()
    # CamelCase to snake_case, so NoShow and NO_SHOW meet in the middle.
    out = []
    for i, ch in enumerate(name):
        if ch.isupper() and i > 0:
            out.append("_")
        out.append(ch.lower())
    return "".join(out)


def canon_guard(raw):
    name = raw.strip()
    if name in ("None", "nullptr", "NULL", "always", ""):
        return "none"
    m = re.fullmatch(r"Some\((.+)\)", name)
    if m:
        name = m.group(1).strip()
    if name == "always":
        return "none"
    return name


def read_c():
    """claim_decide's chain: `if (guard(in)) { return ARM(n, CODE); }` then a bare return."""
    text = (P02 / "c" / "claim.c").read_text(encoding="utf-8")
    body = text.split("claim_decision_t claim_decide(", 1)[1]

    arms = []
    guarded = re.findall(
        r"if\s*\(\s*(\w+)\s*\(\s*in\s*\)\s*\)\s*\{\s*return\s+ARM\(\s*(\d+)\s*,\s*(\w+)\s*\)",
        body,
    )
    for guard, num, code in guarded:
        arms.append((int(num), canon_guard(guard), canon_code(code)))

    # The fall-through: a return not preceded by an if on the same statement.
    tail = re.search(r"\n\s*return\s+ARM\(\s*(\d+)\s*,\s*(\w+)\s*\);\s*\n\}", body)
    if tail:
        arms.append((int(tail.group(1)), "none", canon_code(tail.group(2))))

    arms.sort(key=lambda a: a[0])
    return [(g, c) for _, g, c in arms]


def read_cpp():
    """kCascade's CLAIM_ARM(guard, Code, guarded) entries, skipping the macro definition."""
    text = (P02 / "cpp" / "claim.hpp").read_text(encoding="utf-8")
    body = text.split("kCascade = {{", 1)[1].split("}};", 1)[0]

    arms = []
    flags = []
    for guard, code, flag in re.findall(
        r"CLAIM_ARM\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(true|false)\s*\)", body
    ):
        arms.append((canon_guard(guard), canon_code(code)))
        flags.append(flag == "true")
    return arms, flags


def read_rust():
    """CASCADE's `Arm { guard: ..., code: Code::... }` entries."""
    text = (P02 / "rust" / "claim_core.rs").read_text(encoding="utf-8")
    body = text.split("pub static CASCADE:", 1)[1].split("];", 1)[0]

    arms = []
    for guard, code in re.findall(
        r"Arm\s*\{\s*guard:\s*(None|Some\(\s*\w+\s*\))\s*,\s*code:\s*(Code::\w+)\s*\}", body
    ):
        arms.append((canon_guard(guard), canon_code(code)))
    return arms


def main():
    problems = 0

    c = read_c()
    cpp, cpp_flags = read_cpp()
    rust = read_rust()

    for name, arms in (("C", c), ("C++", cpp), ("Rust", rust)):
        if len(arms) != ARMS_EXPECTED:
            print("FAIL %s has %d arms, expected %d" % (name, len(arms), ARMS_EXPECTED))
            problems += 1

    # Arm for arm, in order.
    for i in range(max(len(c), len(cpp), len(rust))):
        got = {}
        for name, arms in (("C", c), ("C++", cpp), ("Rust", rust)):
            got[name] = arms[i] if i < len(arms) else ("missing", "missing")
        if len({v for v in got.values()}) != 1:
            print("FAIL arm %d differs between the languages:" % (i + 1))
            for name in ("C", "C++", "Rust"):
                print("       %-5s guard %-24s code %s" % (name, got[name][0], got[name][1]))
            problems += 1

    # The C++ duplicates the guarded fact as a bool, which is checked here rather than at
    # compile time: g++ refuses a function-pointer comparison in a constant expression under
    # the sanitisers, which cost three red CI runs on Tuesday 6 October 2026.
    for i, (arm, flag) in enumerate(zip(cpp, cpp_flags)):
        if (arm[0] != "none") != flag:
            print("FAIL C++ arm %d is marked guarded=%s and its guard is %r"
                  % (i + 1, str(flag).lower(), arm[0]))
            problems += 1

    # Exactly one unguarded arm, and it is last.
    for name, arms in (("C", c), ("C++", cpp), ("Rust", rust)):
        unguarded = [i for i, (g, _) in enumerate(arms) if g == "none"]
        if unguarded != [len(arms) - 1]:
            print("FAIL %s: the unguarded arms are at %r, expected only the last"
                  % (name, [i + 1 for i in unguarded]))
            problems += 1

    # Every code the policy declares is produced by some arm.
    produced = {code for _, code in c}
    declared = {
        "invisible", "rejected", "booked", "grace",
        "no_show", "walkin", "brb", "free",
    }
    for code in sorted(declared - produced):
        print("FAIL no arm produces %s, so the policy cannot reach it" % code)
        problems += 1

    if problems == 0:
        print("read from %s" % P02)
        print("the three cascades agree on all %d arms, in order, "
              "on guard and on code" % ARMS_EXPECTED)
        print("exactly one arm is unguarded in each, and it is the last")
        print("all %d codes are produced, the fall-through included" % len(declared))
        for i, (guard, code) in enumerate(c):
            print("  %d  %-24s -> %s" % (i + 1, guard, code))

    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
