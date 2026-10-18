#!/usr/bin/env python3
"""Nothing third-party is copied into this tree, and every external source is pinned.

    python scripts/check_vendoring.py

This is P05's seventh acceptance criterion as a gate rather than as a sentence. The criterion
was written as "the licence travels with the code", which assumed the vendor's source would be
committed here with its licence beside it. It is not: it is cloned at a revision and compiled
where it lies, so the criterion restates as two rules, and both are checkable on a host with no
hardware and no clone.

**Why it needs a gate at all.** The project's own build file said the pin was "what was built
against" and that "the version becomes part of the build rather than part of the history". On
Sunday 18 October 2026 the pin was found to live in a comment, which no cmake command reads, so
the build compiled whatever revision happened to be checked out. The rule was named in prose and
enforced by nothing, which is this volume's most repeated failure. Three rules are enforced here:

    the pin is a value          an external source declares its revision as a cmake cache value
                                that the build can compare against, not only in a comment, and it
                                is a full forty-character commit rather than a branch or a tag
    the source stays outside    a cache path default may not resolve inside this repository, since
                                that is how a clone quietly becomes a copy
    nothing is copied in        every C and C++ source under projects/ names its own repository
                                path in its opening comment, which is this tree's convention for
                                61 of 61 such files. A file carrying somebody else's header does
                                not, so a vendored drop fails here on its first line

The third rule is deliberately a house-style check rather than a search for vendor names. A list
of third-party filenames goes stale and says nothing about the next library; a file that does not
introduce itself is the general case. Rust is excluded on purpose: those files carry `//!` module
documentation describing their role, which is that language's convention and is not a defect.

Exit status is 0 when the tree is as intended and 1 otherwise, naming every file that is not.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# A path this repository would be publishing rather than referencing. A cache default naming any
# of these is a copy waiting to happen.
INSIDE_MARKERS = ("${CMAKE_CURRENT_SOURCE_DIR}", "${CMAKE_SOURCE_DIR}", "${ZEPHYR_BASE}/../..")

SELF_NAMING = (".c", ".h", ".cpp", ".hpp")

FORTY_HEX = re.compile(r"\b[0-9a-f]{40}\b")
CACHE_PATH = re.compile(r"""set\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s+"([^"]*)"\s*\n?\s*CACHE\s+PATH""",
                        re.S)
CACHE_STRING = re.compile(
    r"""set\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s+"([^"]*)"\s*\n?\s*CACHE\s+STRING""", re.S)


def tracked(pattern: str) -> list[str]:
    out = subprocess.run(["git", "ls-files", pattern], cwd=ROOT,
                         capture_output=True, text=True, check=True)
    return [line for line in out.stdout.splitlines() if line]


def strip_comments(text: str) -> str:
    """cmake comments, so a rule written in prose cannot satisfy a rule about values."""
    return "\n".join(re.sub(r"#.*$", "", line) for line in text.splitlines())


def check_external_sources(problems: list[str]) -> int:
    """Every project naming an external source declares a pinned revision the build can read."""
    checked = 0
    for rel in tracked("projects/*/CMakeLists.txt") + tracked("projects/*/*/CMakeLists.txt"):
        text = (ROOT / rel).read_text(encoding="utf-8", errors="replace")
        code = strip_comments(text)

        paths = CACHE_PATH.findall(code)
        if not paths:
            continue
        checked += 1

        for name, value in paths:
            if any(m in value for m in INSIDE_MARKERS):
                problems.append(
                    "%s: %s defaults to %s, which is inside this repository. An external source "
                    "is referenced, never published here." % (rel, name, value))

        # The revision has to be a value, not a comment. A comment is what this check exists for.
        in_code = FORTY_HEX.findall(code)
        in_comments_only = [h for h in FORTY_HEX.findall(text) if h not in in_code]
        strings = {n: v for n, v in CACHE_STRING.findall(code)}
        pinned = [v for v in strings.values() if FORTY_HEX.fullmatch(v.strip())]

        if not pinned:
            if in_comments_only:
                problems.append(
                    "%s: names an external source and its revision %s appears only in a comment, "
                    "which no cmake command reads. Declare it as a CACHE STRING the build "
                    "compares the clone against." % (rel, in_comments_only[0][:12]))
            else:
                problems.append(
                    "%s: names an external source and no forty-character commit anywhere. A "
                    "branch or a tag moves, so what was built against it cannot be stated."
                    % rel)
        for n, v in strings.items():
            v = v.strip()
            if "COMMIT" in n.upper() and not FORTY_HEX.fullmatch(v):
                problems.append(
                    "%s: %s is \"%s\", which is not a forty-character commit. A branch or tag "
                    "moves underneath a published figure." % (rel, n, v))
    return checked


def check_nothing_copied_in(problems: list[str]) -> int:
    """Every C and C++ source under projects/ introduces itself by its own path."""
    checked = 0
    for rel in tracked("projects/*"):
        if not rel.endswith(SELF_NAMING):
            continue
        path = ROOT / rel
        if not path.is_file():
            continue
        checked += 1
        head = "".join(path.read_text(encoding="utf-8", errors="replace").splitlines(True)[:6])
        if rel not in head:
            problems.append(
                "%s: does not name its own path in its first six lines. Either it is a file "
                "copied in from elsewhere, which this repository does not publish, or it is ours "
                "and is missing the opening comment every sibling has." % rel)
    return checked


def main() -> int:
    problems: list[str] = []
    builds = check_external_sources(problems)
    sources = check_nothing_copied_in(problems)

    if problems:
        print("Vendoring and pinning problems:")
        for p in problems:
            print("  " + p)
        return 1

    print("external sources: %d build file(s) naming one, each pinned to a full commit the build "
          "can check" % builds)
    print("copied-in source: none; %d C and C++ file(s) under projects/ all name their own path"
          % sources)
    return 0


if __name__ == "__main__":
    sys.exit(main())
