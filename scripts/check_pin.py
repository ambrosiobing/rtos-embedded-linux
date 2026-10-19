#!/usr/bin/env python3
"""Refuse to build against a clone that is not at the revision a project pins.

    python scripts/check_pin.py --path ~/src/FreeRTOS-Kernel --tag V11.1.0 \
        --url https://github.com/FreeRTOS/FreeRTOS-Kernel.git --what "the FreeRTOS kernel"
    python scripts/check_pin.py --path ~/src/firmware-nucleo-h7 \
        --commit 1d6d10f826ee0728ded6533defc468baa2ff0f2a --url ... --what "..."

WHY THIS EXISTS. P05's build file carried two sentences saying its vendor pin was "what was built
against" and that "the version becomes part of the build rather than part of the history". On
Sunday 18 October 2026 the pin was found to live in a comment that no command read, so the build
compiled whatever revision the clone happened to be at. That was fixed in cmake for P05, and the
three make projects that clone the FreeRTOS kernel, and in one case a sibling repository, were
left reporting a mismatch rather than refusing, and only when their fetch target was run rather
than on an ordinary build. Joseph decided on Monday 19 October 2026 that one rule should apply
across the set.

ONE IMPLEMENTATION RATHER THAN THE SAME SHELL IN THREE MAKEFILES. The checks differ only in a
path and a revision, and three copies of twenty lines of shell is three places for them to drift.
This also makes the rule testable away from the bench: the suite beside it builds temporary git
repositories and moves them off the pin on purpose.

A TAG AND A COMMIT ARE BOTH ACCEPTED, and they are not the same promise. `--commit` wants all
forty characters and is exact. `--tag` is what the kernel projects use, because the upstream
publishes releases that way and that is a deliberate convention rather than an oversight; it is
satisfied either by HEAD being the commit the tag resolves to, or by HEAD being exactly at a tag
of that name. A tag can be moved by whoever owns it, which a commit cannot, and that difference
is recorded here rather than argued in three Makefiles.

REFUSING IS THE POINT, and the escape hatch is explicit. `--allow-any` warns, says in the same
breath that nothing measured from the build may be quoted beside a figure taken at the pin, and
exits zero. A build that silently used another revision is the thing this prevents; a build that
deliberately does so, and says so, is a reasonable thing to want.

Exit status is 0 when the clone is at the pin, or when --allow-any is given, and 1 otherwise.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

FORTY_HEX = re.compile(r"^[0-9a-f]{40}$")


def git(path: Path, *args: str) -> str | None:
    """A git command's output in that clone, or None if git could not answer."""
    try:
        out = subprocess.run(("git", "-C", str(path)) + args,
                             capture_output=True, text=True, check=True)
    except (OSError, subprocess.CalledProcessError):
        return None
    return out.stdout.strip()


def say(lines: list[str]) -> None:
    for line in lines:
        print(line)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--path", required=True, help="the clone to check")
    p.add_argument("--what", default="the external source", help="what to call it in messages")
    p.add_argument("--url", default="", help="where it is cloned from, for the remedy")
    p.add_argument("--tag", default="", help="the release tag this project is written against")
    p.add_argument("--commit", default="", help="the forty-character commit, for an exact pin")
    p.add_argument("--allow-any", action="store_true",
                   help="build against whatever is checked out, and say so")
    a = p.parse_args()

    if bool(a.tag) == bool(a.commit):
        print("check_pin: give exactly one of --tag and --commit")
        return 1
    if a.commit and not FORTY_HEX.match(a.commit):
        print("check_pin: --commit is %r, which is not forty hexadecimal characters. An "
              "abbreviated hash can become ambiguous as the other repository grows." % a.commit)
        return 1

    path = Path(a.path).expanduser()
    pin = a.commit or a.tag

    if not path.is_dir():
        say(["%s is not at %s." % (a.what, path),
             "  git clone %s%s \"%s\"" % (
                 "--depth 1 --branch %s " % a.tag if a.tag else "", a.url, path)]
            + (["  git -C \"%s\" checkout %s" % (path, a.commit)] if a.commit else []))
        return 1

    head = git(path, "rev-parse", "HEAD")
    if head is None:
        say(["%s at %s is not a git clone, so the revision it carries cannot be read." %
             (a.what, path),
             "A copy is not a clone: this repository states the revision every figure came from,",
             "so it has to be able to ask."])
        return 1

    # The exact-commit form first, then the tag form, then the tag by name. A shallow clone made
    # with --branch <tag> carries that tag, so the last check succeeds there too.
    at_pin = False
    if a.commit:
        at_pin = head == a.commit
    else:
        want = git(path, "rev-parse", "--verify", "--quiet", a.tag + "^{commit}")
        at_pin = (want is not None and want == head) or \
                 git(path, "describe", "--tags", "--exact-match", "HEAD") == a.tag

    if at_pin:
        print("%s is at the pinned %s" % (a.what, pin))
        return 0

    where = git(path, "describe", "--tags", "--always", "HEAD") or head

    if a.allow_any:
        say(["WARNING %s is at %s, not the pinned %s, and --allow-any was given." %
             (a.what, where, pin),
             "  This build is going ahead. Nothing measured from it may be quoted beside a",
             "  figure taken at the pin without saying so."])
        return 0

    say(["%s is not at the revision this project is written against." % a.what,
         "  clone is at   %s" % where,
         "  pinned here   %s" % pin,
         "Every figure this project publishes was measured against the pinned one, so the build",
         "refuses rather than quietly producing numbers that belong to another revision:"]
        + (["  git -C \"%s\" fetch --tags --depth 1 %s tag %s" % (path, a.url or "origin", a.tag),
            "  git -C \"%s\" checkout %s" % (path, a.tag)] if a.tag else
           ["  git -C \"%s\" fetch %s" % (path, a.url or "origin"),
            "  git -C \"%s\" checkout %s" % (path, a.commit)])
        + ["To build against the clone's revision on purpose, add ALLOW_ANY_REVISION=1, which",
           "warns instead and marks the build as unpinned."])
    return 1


if __name__ == "__main__":
    sys.exit(main())
