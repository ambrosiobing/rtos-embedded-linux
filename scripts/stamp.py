#!/usr/bin/env python3
"""What the image is, written at every link rather than at a translation unit's compile.

    python scripts/stamp.py --out <path to a generated .c file>

WHY THIS SCRIPT EXISTS, AND IT IS A CORRECTION. Every firmware in this repository prints
`__DATE__` and `__TIME__` in its banner, added at `0facc30` on Friday 9 October 2026 so that a
stale flash could be told from a reproduction. The reasoning written down with it was that those
two macros "need nothing from the build system and their whole purpose is to differ between
builds". **That is false in a way that cost a reading.** They differ between compiles of the one
file that contains them, and a build that changes any other file does not recompile it.

On Saturday 17 October 2026 at 16:51 a P05 image was built after an edit to the driver, flashed,
and its banner read 15:42. The image was genuinely new, by 168 bytes of a different link, and the
stamp said the opposite. The check that existed to catch a stale flash reported one that was not
stale, which is the worse of the two failures: the first wastes a build and the second discards a
real result.

WHAT IT WRITES, AND WHY TWO FIELDS RATHER THAN ONE. A time says when, a commit says what, and
only the second survives being asked a second time:

    the time      distinguishes two builds of the same source, which is what a bench does all
                  afternoon. Written in full, weekday included, because this is a human reading a
                  console and the house rule for dates does not stop at a program's output
    the commit    distinguishes two sources, and `-dirty` says the build came from an edit that
                  is in no history, which is the state most bench builds are actually in

Both are regenerated on every build by the custom target in the project's CMakeLists.txt, so the
one translation unit that carries them recompiles every time and the link that follows is stamped
with itself.

THE WEEKDAY IS COMPUTED AND THE NAMES ARE SPELLED OUT HERE rather than taken from `%A`, which
reads the machine's locale. This builds in WSL on a laptop whose Windows side is not English and
a month name that changes language between two machines would make two logs hard to compare.
"""
from __future__ import annotations

import argparse
import datetime
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

DAYS = ("Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday")
MONTHS = ("January", "February", "March", "April", "May", "June",
          "July", "August", "September", "October", "November", "December")


def when() -> str:
    now = datetime.datetime.now()
    return "%s %d %s %d, %02d:%02d:%02d" % (
        DAYS[now.weekday()], now.day, MONTHS[now.month - 1], now.year,
        now.hour, now.minute, now.second)


def git(*args: str) -> str | None:
    """A git command's output, or None when there is no git and no history here.

    A build from a tarball, or from a tree with no git, is a real case and it gets a string that
    says so rather than an empty one: a banner reading nothing is a banner a reader assumes is
    broken.
    """
    try:
        out = subprocess.run(("git",) + args, cwd=ROOT, capture_output=True, text=True,
                             check=True)
    except (OSError, subprocess.CalledProcessError):
        return None
    return out.stdout


def commit(scope: str | None) -> str:
    """The commit this image was built from, and whether the source on disk still matches it.

    THE DIRTY MARKER IS COMPUTED HERE AND NOT TAKEN FROM `git describe --dirty`, which was the
    first version of this script and was wrong in the direction that matters. That option reads
    the index against the working tree, so it reports a modified tracked file and says nothing
    about an untracked one. Every file in the project this was written for began untracked, and
    a build from a new file would have printed a clean commit hash: a stamp asserting the image
    matches a published history when it contains code that is in no history at all.

    WHAT THE MARKER COVERS is the project directory passed as `--scope`, because that is what
    went into this image, and a repository holding twenty projects would otherwise mark every
    image dirty whenever any of them was being edited. **What it does not cover** is the
    vendor's library, which is cloned outside this repository and pinned by hash in the
    project's CMakeLists.txt, so the hash in that file is the record of it and this stamp is not.
    """
    head = git("rev-parse", "--short", "HEAD")
    if head is None:
        return "no git here, so the source cannot be named"
    head = head.strip()
    if not head:
        return "git named no commit"

    status = git("status", "--porcelain", "--", scope) if scope else git("status", "--porcelain")
    if status is None:
        return head + " plus whatever git could not be asked about"
    changed = [line for line in status.splitlines() if line.strip()]
    if not changed:
        return head
    return "%s plus %d edited or untracked file(s)" % (head, len(changed))


def c_string(text: str) -> str:
    """Escaped for C, because a stamp is not a place to discover that a quote breaks a build."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, help="the .c file to write")
    parser.add_argument("--scope", default=None,
                        help="the directory whose edits count as this image's, "
                             "e.g. projects/05-the-zones")
    args = parser.parse_args()

    body = (
        "/* Generated by scripts/stamp.py at every build. Not tracked, and not edited: an edit\n"
        " * here is overwritten by the next build and the reason is in that script. */\n"
        "const char build_stamp_when[] = \"%s\";\n"
        "const char build_stamp_commit[] = \"%s\";\n"
    ) % (c_string(when()), c_string(commit(args.scope)))

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(body, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
