#!/usr/bin/env python3
"""What the published tree may contain: the file types, and the size ceiling.

    python scripts/check_tree.py

Both rules lived inside .github/workflows/checks.yml as shell until Saturday 17 October 2026,
and both are the same rules with the same reasons. They were moved here for one reason: **a gate
that can only be run by pushing is a gate you discover by email.** On that Saturday eight
consecutive pushes failed on the first of them, for one file, and the author found out from a
mailbox rather than from a terminal. Nothing about the rules changed in the move; the comments
below are the originals, because their reasoning is the part worth keeping.

Exit status is 0 when the tree is as intended and 1 otherwise, naming every file that is not.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Markdown, SVG and the scripts that produce them are what a clone is for; a reader should not be
# asked to download anything.
#
# Extended on Sunday 5 October 2026, when P01 brought the first code. The purpose of this guard is
# unchanged and is not about source types: it keeps out the PDF, the HTML and the LaTeX. Source
# files are what the repository is for, so the allowlist names the ones the projects carry.
#
# EXTENDED AGAIN ON FRIDAY 9 OCTOBER 2026, TO ADMIT ONE BINARY TYPE, AND THAT ONE IS A REAL
# RELAXATION RATHER THAN A CLARIFICATION. A reader should not be asked to download anything and a
# GIF is a download; the clause after it says the purpose is to keep out the PDF, the HTML and the
# LaTeX, which a GIF is not. The two halves disagreed about the case, so it was decided rather
# than deduced: P04's criterion 5 needs a wire between two boards and no drawing proves a wire is
# fitted. A dated moving picture of the bench does. The ceiling below was written in the same
# commit, because admitting a type without one is how a documentation repository becomes a
# download.
#
# EXTENDED ON SATURDAY 17 OCTOBER 2026 TO ADMIT `.yaml`, AND THAT ONE IS A CLARIFICATION. P05
# writes a devicetree binding, which is the file that describes a part to the operating system,
# and the operating system requires it to be spelled `.yaml`. The allowlist already admits
# `.conf` and `.overlay`, which are the other two description files a project here carries, so
# this is the third of that set rather than a new kind of thing. The purpose of the guard is
# untouched: no PDF, no HTML, no LaTeX.
ALLOWED = re.compile(
    r"(\.(md|svg|gif|py|yml|yaml|cff|c|h|cpp|hpp|rs|toml|conf|overlay)$"
    r"|^LICENSE$"
    r"|^\.git(ignore|attributes)$"
    r"|(^|/)Makefile$"
    r"|(^|/)CMakeLists\.txt$)"
)

# A temporary or widening change needs its limit written at the moment it is made, or nobody
# writes it later. Four megabytes admits the one GIF that prompted it and refuses the careless
# one: a full-resolution capture of the same six seconds is over fifty.
#
# If this ever fails on a file that genuinely belongs, the answer is almost always to make the
# file smaller, not the number bigger. The GIF it was written around came down from 5.47 MB by
# dropping to eight frames a second and sixty-four colours, with no loss of anything a reader
# needs to see.
CEILING = 4 * 1024 * 1024


def tracked() -> list[str]:
    out = subprocess.run(["git", "ls-files"], cwd=ROOT, capture_output=True, text=True, check=True)
    return [line for line in out.stdout.splitlines() if line]


def main() -> int:
    files = tracked()
    problems = 0

    odd = [f for f in files if not ALLOWED.search(f)]
    if odd:
        problems += len(odd)
        print("Files of a type this repository does not publish:")
        for f in odd:
            print(f"  {f}")
    else:
        print("file types: Markdown, SVG, GIF, the descriptions, the sources, the scripts, "
              "the workflow, the licence and the citation, as intended")

    big = []
    for f in files:
        path = ROOT / f
        if path.is_file() and path.stat().st_size > CEILING:
            big.append((path.stat().st_size, f))
    if big:
        problems += len(big)
        print("Files over four megabytes are committed, in bytes and path:")
        for size, f in sorted(big, reverse=True):
            print(f"  {size} {f}")
        print()
        print("Git keeps a blob even after a later commit removes it, so this is permanent in")
        print("every clone. Make the file smaller.")
    else:
        print(f"size: nothing over {CEILING} bytes, as intended")

    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
