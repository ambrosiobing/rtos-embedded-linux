#!/usr/bin/env python3
"""Nothing third-party is copied into this tree, and every external source is pinned.

    python scripts/check_vendoring.py

This is P05's seventh acceptance criterion as a gate rather than as a sentence. The criterion
was written as "the licence travels with the code", which assumed the vendor's source would be
committed here with its licence beside it. It is not: it is cloned at a revision and compiled
where it lies, so the criterion restates as the rules below, all of which are checkable on a host
with no hardware and no clone.

**Why it needs a gate at all.** The project's own build file said the pin was "what was built
against" and that "the version becomes part of the build rather than part of the history". On
Sunday 18 October 2026 the pin was found to live in a comment, which no cmake command reads, so
the build compiled whatever revision happened to be checked out. The rule was named in prose and
enforced by nothing, which is this volume's most repeated failure.

    the pin is a value          an external source declares its revision where the build reads it,
                                a cmake cache value or a make variable, never only in a comment
    a commit is whole           a variable whose name says COMMIT holds all forty characters. A
                                tag is accepted where the variable is named for a tag, which is a
                                deliberate convention in the make projects and not this script's
                                business to overrule
    the source stays outside    a path default may not resolve inside this repository, since that
                                is how a clone quietly becomes a copy
    nothing is copied in        every C and C++ source under projects/ names its own repository
                                path in its opening comment, which is this tree's convention for
                                61 of 61 such files. A file carrying somebody else's header does
                                not, so a vendored drop fails here on its first line

**What this script does not check, said here so the gap is not mistaken for coverage.** Whether
the clone on the machine doing the build is actually at the pin. That needs the clone, so it
belongs in the build: P05's CMakeLists asks git and refuses to configure against another
revision, while the three make projects only report a mismatch, and only when their fetch target
is run rather than on an ordinary build.

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
    """Tracked paths matching one pathspec, de-duplicated and in a stable order.

    ONE PATTERN IS ENOUGH, AND TWO IS A BUG. A git pathspec matches `*` across directory
    separators, so `projects/*/Makefile` already finds `projects/NN/freertos/Makefile`. The first
    version of this script passed both that and `projects/*/*/Makefile` and reported every
    finding twice, which is how the duplication was noticed. The de-duplication stays anyway, so
    a future second pattern cannot reintroduce it quietly.
    """
    out = subprocess.run(["git", "ls-files", pattern], cwd=ROOT,
                         capture_output=True, text=True, check=True)
    seen, paths = set(), []
    for line in out.stdout.splitlines():
        if line and line not in seen:
            seen.add(line)
            paths.append(line)
    return paths


def strip_comments(text: str) -> str:
    """cmake comments, so a rule written in prose cannot satisfy a rule about values."""
    return "\n".join(re.sub(r"#.*$", "", line) for line in text.splitlines())


def check_external_sources(problems: list[str]) -> int:
    """Every project naming an external source declares a pinned revision the build can read."""
    checked = 0
    for rel in tracked("projects/*/CMakeLists.txt"):
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


MAKE_ASSIGN = re.compile(r"(?m)^([A-Z][A-Z0-9_]*)\s*\?=\s*(.*?)\s*$")
MAKE_INSIDE = ("$(CURDIR)", "$(PWD)", "./", "../")


def check_makefiles(problems: list[str]) -> int:
    """The same two rules for the projects that clone with make rather than with cmake.

    THIS FUNCTION IS THE REASON TO DISTRUST THE FIRST VERSION OF THIS SCRIPT. It looked only at
    `CMakeLists.txt` and reported "1 build file naming an external source", which was true and
    read as complete. Three Makefiles clone a kernel and a sibling repository, and none of them
    was being checked: a gate that covers one of four is the shape this repository has already
    paid for once, where a passing pipeline said nothing about the targets no job enumerated.

    A TAG IS ACCEPTED WHERE A TAG IS WHAT THE VARIABLE IS CALLED. These Makefiles pin the kernel
    at `V11.1.0`, which is a release tag and a deliberate choice, not an oversight. A tag is
    mutable in principle and conventionally fixed in practice, and overruling that convention is
    not this script's business. What it does insist on is that a variable whose name says
    `COMMIT` holds a whole commit: an abbreviated hash can become ambiguous as a repository
    grows, and `git checkout` on an ambiguous prefix does not resolve.
    """
    checked = 0
    for rel in tracked("projects/*/Makefile"):
        text = (ROOT / rel).read_text(encoding="utf-8", errors="replace")
        code = "\n".join(re.sub(r"#.*$", "", line) for line in text.splitlines())
        assigns = dict(MAKE_ASSIGN.findall(code))
        if not any(k.endswith("_URL") for k in assigns):
            continue
        checked += 1

        for name, value in assigns.items():
            if name.endswith("_URL"):
                continue
            if any(m in value for m in MAKE_INSIDE):
                problems.append(
                    "%s: %s defaults to %s, which is inside this repository. An external source "
                    "is referenced, never published here." % (rel, name, value))
            if name.endswith("_COMMIT") and not FORTY_HEX.fullmatch(value):
                problems.append(
                    "%s: %s is \"%s\", which is not a whole commit. An abbreviated hash can "
                    "become ambiguous as the other repository grows, and a checkout on an "
                    "ambiguous prefix does not resolve. Write all forty characters."
                    % (rel, name, value))

        # AND THE PIN HAS TO BE CHECKED, not merely declared. A project can hold a perfectly good
        # revision in a variable and never compare it against the clone, which is exactly what
        # these three did until Monday 19 October 2026: one printed a note when its fetch target
        # was run and the others compared nothing at all. Removing the dependency again would be
        # a one-line change that nothing else would notice.
        if "check_pin.py" not in code:
            problems.append(
                "%s: clones an external source and never calls scripts/check_pin.py, so the "
                "revision above is a statement of intent rather than a property of the build."
                % rel)

        # Each external source needs a pin that the build can read, by the same rule cmake gets.
        for name in [k for k in assigns if k.endswith("_URL")]:
            stem = name[: -len("_URL")]
            if not any(assigns.get(stem + s) for s in ("_TAG", "_COMMIT", "_REF")):
                in_comment = FORTY_HEX.search(text)
                problems.append(
                    "%s: %s names a source to clone and no %s_TAG or %s_COMMIT holds its "
                    "revision%s. What was built against it cannot be stated."
                    % (rel, name, stem, stem,
                       ", though a commit appears in a comment" if in_comment else ""))
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
    makes = check_makefiles(problems)
    sources = check_nothing_copied_in(problems)

    if problems:
        print("Vendoring and pinning problems:")
        for p in problems:
            print("  " + p)
        return 1

    print("external sources: %d cmake and %d make build file(s) naming one, each with its "
          "revision as a value the build reads" % (builds, makes))
    print("copied-in source: none; %d C and C++ file(s) under projects/ all name their own path"
          % sources)
    return 0


if __name__ == "__main__":
    sys.exit(main())
