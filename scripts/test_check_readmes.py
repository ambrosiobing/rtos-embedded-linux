#!/usr/bin/env python3
"""Does check_readmes.py refuse each of the four things it exists to refuse?

    python3 scripts/test_check_readmes.py

Synthetic trees, one per rule, each satisfying the other three so that a refusal names one cause.
Four rules that have only ever passed are four assertions, which is the defect the checker itself
was written about.

The fifth case is the one that caught a real mistake. Rule four began as a substring test for
"there is no code here yet", and the commit that corrected the front page quotes that sentence as
a thing the page used to say. The gate refused its own fix. So a tree that quotes the claim must
pass while a tree that makes it must fail, and both are checked here.
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "scripts" / "check_readmes.py"

FRONT_OK = """# A synthetic volume

## Checks

    python scripts/lint.py        house rules
    python scripts/check_links.py every relative link

Five parts carry code and the rest are written and not started.
"""

WORKFLOW = """name: checks
on: [push]
jobs:
  gates:
    runs-on: ubuntu-latest
    steps:
      - run: python scripts/lint.py
      - run: python scripts/check_links.py
"""


def build(root: Path, *, front=FRONT_OK, workflow=WORKFLOW,
          started_has_page=False, started_has_code=False) -> Path:
    """A tree with one started project and one not-started project.

    The keyword arguments break exactly one rule each, which is what makes a refusal readable.
    """
    root.mkdir(parents=True)
    (root / "README.md").write_text(front, encoding="utf-8")
    wf = root / ".github" / "workflows"
    wf.mkdir(parents=True)
    (wf / "checks.yml").write_text(workflow, encoding="utf-8")

    started = root / "projects" / "01-started"
    (started / "docs").mkdir(parents=True)
    (started / "src").mkdir(parents=True)
    (started / "src" / "main.c").write_text("int main(void){return 0;}\n", encoding="utf-8")
    (started / "docs" / "DESIGN.md").write_text("# design\n", encoding="utf-8")
    (started / "README.md").write_text(
        "# P01\n\nStarted. See [docs/DESIGN.md](docs/DESIGN.md).\n", encoding="utf-8")

    quiet = root / "projects" / "02-quiet"
    quiet.mkdir(parents=True)
    (quiet / "README.md").write_text(
        "# P02\n\nStatus: not started. Nothing in this directory has been built.\n",
        encoding="utf-8")
    if started_has_page:
        (quiet / "docs").mkdir()
        (quiet / "docs" / "layout.md").write_text("# a decision\n", encoding="utf-8")
    if started_has_code:
        (quiet / "src").mkdir()
        (quiet / "src" / "x.c").write_text("int x;\n", encoding="utf-8")
    return root


def check(root: Path, work: Path):
    r = subprocess.run([sys.executable, str(SCRIPT), str(root)],
                       cwd=str(work), capture_output=True, text=True)
    return r.returncode, r.stdout


def main() -> int:
    if not SCRIPT.exists():
        print("REFUSED: %s is not there." % SCRIPT.relative_to(ROOT))
        return 2

    results = []
    with tempfile.TemporaryDirectory(prefix="readmegate") as tmp:
        work = Path(tmp)

        rc, out = check(build(work / "clean"), work)
        results.append(("a tree where every rule holds passes", rc == 0))
        results.append(("and says so", "claims no less and no more" in out))

        # Rule 1: a prose page the project README does not name.
        root = build(work / "unnamed")
        (root / "projects" / "01-started" / "docs" / "ORPHAN.md").write_text(
            "# nobody points here\n", encoding="utf-8")
        rc, out = check(root, work)
        results.append(("an unnamed prose page refuses", rc == 1))
        results.append(("and the refusal names the page", "ORPHAN.md" in out))

        # Rule 2, both halves.
        rc, out = check(build(work / "quiet-with-page", started_has_page=True), work)
        results.append(("not started with a recorded decision refuses", rc == 1))
        results.append(("and names the page", "layout.md" in out))

        rc, out = check(build(work / "quiet-with-code", started_has_code=True), work)
        results.append(("not started with code refuses", rc == 1))
        results.append(("and says how many files", "code file(s)" in out))

        # Rule 3: a gate the workflow runs and the front page omits.
        more = WORKFLOW + "      - run: python scripts/check_vendoring.py\n"
        rc, out = check(build(work / "ungated", workflow=more), work)
        results.append(("a gate the front page omits refuses", rc == 1))
        results.append(("and names the gate", "check_vendoring.py" in out))

        # Rule 4: the claim asserted, which must fail.
        denies = FRONT_OK + "\nThere is no code here yet, and the READMEs say so.\n"
        rc, out = check(build(work / "denies", front=denies), work)
        results.append(("denying code that exists refuses", rc == 1))

        # Rule 4 again: the claim quoted, which must pass. This is the case that caught the
        # checker refusing the commit that fixed the page it was checking.
        quotes = FRONT_OK + ('\nThis page said "there is no code here yet" until it was '
                             'corrected.\n')
        rc, out = check(build(work / "quotes", front=quotes), work)
        results.append(("quoting the claim as history passes", rc == 0))

        rc, out = check(work / "not-a-tree", work)
        results.append(("a path with no front page refuses with its own code", rc == 2))

    for label, ok in results:
        print("  %-52s %s" % (label, "yes" if ok else "NO"))
    passed = sum(1 for _, ok in results if ok)
    print()
    print("%d of %d checks pass" % (passed, len(results)))
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
