#!/usr/bin/env python3
"""Does every README still describe the tree it sits in? Four rules, checked rather than trusted.

    python scripts/check_readmes.py

WHY THIS EXISTS. On Sunday 8 November 2026 the front page of this repository carried a section
headed "There is no code here yet", under which the tree held 36 C files, 21 headers, ten Rust
files, nine devicetree overlays and five Makefiles, and three parts had run on the board. The
same page listed four Python scripts where `scripts/` held nineteen, named three gates where the
workflow ran thirteen, and stated that every file here is Markdown, SVG, one of those four
scripts, the workflow, the licence or the citation file, followed by "and the workflow checks
it", which it did not.

**None of that was a lie when it was written.** Each sentence was true on Friday 2 October 2026
and was falsified by work that had no reason to touch the front page. That is the whole failure
mode: a README goes stale through other people's correct commits, silently, and the staler it is
the less anybody reads it.

**A rule named in prose and enforced by nothing is this volume's most repeated defect**, which
projects/05-the-zones/CMakeLists.txt says in those words about a different rule. The habit of
updating affected READMEs is such a rule, so it is enforced here instead of promised.

THE FOUR RULES, AND EACH IS MECHANICAL ON PURPOSE. A gate that needed judgement would be argued
with:

    1  a prose page under projects/NN/docs/ is named in that project's README
    2  a README claiming "not started" has no code and no prose page under docs/
    3  every gate the Python workflow runs is named in the front page's Checks section
    4  the front page does not say there is no code while there is code

WHAT THIS DELIBERATELY DOES NOT CHECK is whether a README is well written, whether it names every
header beside a source it already names, or whether its prose is current in any sense a machine
cannot see. Rule 1 is about pages a reader would go looking for, not about files. Three headers
and a Makefile were left unnamed in P05 on purpose the day this was written, because a README
that lists every file stops being read.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

NOT_STARTED = ("not started", "nothing in this directory has been built")
NO_CODE_CLAIMS = ("there is no code here yet", "no project has been built")
CODE_SUFFIXES = (".c", ".cpp", ".rs")

# THE ROOT IS AN ARGUMENT SO THE RULES CAN BE EXERCISED. It defaults to this repository, which is
# the only way it is ever run for real. scripts/test_check_readmes.py points it at synthetic
# trees that break one rule each, because four rules that have only ever passed are four
# assertions.


def project_dirs(root: Path) -> list[Path]:
    projects = root / "projects"
    if not projects.is_dir():
        return []
    return sorted(d for d in projects.iterdir()
                  if d.is_dir() and (d / "README.md").exists())


def prose_pages(project: Path) -> list[Path]:
    """Markdown under docs/, excluding the figure directory, which is drawings and not prose."""
    docs = project / "docs"
    if not docs.is_dir():
        return []
    return sorted(p for p in docs.glob("*.md") if p.is_file())


def code_files(project: Path) -> list[Path]:
    return sorted(p for p in project.rglob("*")
                  if p.is_file() and p.suffix in CODE_SUFFIXES)


def rule_one(root: Path, problems: list[str]) -> None:
    """Every prose page under a project's docs/ is named in that project's README."""
    for project in project_dirs(root):
        readme = (project / "README.md").read_text(encoding="utf-8")
        for page in prose_pages(project):
            if page.name not in readme:
                problems.append(
                    "%s does not name %s, which is a page a reader would go looking for. "
                    "A page nobody is pointed at is a page nobody reads."
                    % ((project / "README.md").relative_to(root), page.name))


def rule_two(root: Path, problems: list[str]) -> None:
    """A README claiming not started has nothing started."""
    for project in project_dirs(root):
        path = project / "README.md"
        head = path.read_text(encoding="utf-8")[:600].lower()
        if not any(claim in head for claim in NOT_STARTED):
            continue
        code = code_files(project)
        pages = prose_pages(project)
        if code:
            problems.append(
                "%s says it is not started and the directory holds %d code file(s), "
                "the first being %s."
                % (path.relative_to(root), len(code), code[0].relative_to(root)))
        if pages:
            problems.append(
                "%s says it is not started and docs/ holds %s. A decision recorded is work "
                "done, so the state line says so rather than reading not started."
                % (path.relative_to(root), ", ".join(p.name for p in pages)))


def workflow_gates(root: Path) -> list[str]:
    """Every scripts/*.py the Python-only workflow runs, in the order it runs them."""
    workflow = root / ".github" / "workflows" / "checks.yml"
    if not workflow.exists():
        return []
    text = workflow.read_text(encoding="utf-8")
    found, seen = [], set()
    for match in re.finditer(r"scripts/([A-Za-z0-9_]+\.py)", text):
        name = match.group(1)
        if name not in seen:
            seen.add(name)
            found.append(name)
    return found


def rule_three(root: Path, problems: list[str]) -> None:
    """Every gate the workflow runs is named on the front page."""
    front = (root / "README.md").read_text(encoding="utf-8")
    missing = [g for g in workflow_gates(root) if g not in front]
    if missing:
        problems.append(
            "README.md does not name %d gate(s) that checks.yml runs: %s. The Checks section is "
            "what a reader runs on a clone, so a gate it omits is a gate nobody runs by hand."
            % (len(missing), ", ".join(missing)))


def asserted(text: str, claim: str) -> bool:
    """Is the claim made here, as opposed to quoted as a thing that used to be said?

    THE FIRST VERSION OF THIS FUNCTION DID NOT EXIST and rule four was a substring test, which
    refused the very commit that fixed the front page: the corrected section records what the
    page used to say, in quotation marks, and a substring test cannot tell a claim from a
    citation of one. That is the fifth time in this repository that a checker has matched prose
    written about its own subject, and the remedy is the same every time, which is to look only
    where the thing being checked takes effect.

    A line that puts the claim inside double quotes is reporting it. Anywhere else it is being
    said.
    """
    for line in text.splitlines():
        lowered = line.lower()
        if claim not in lowered:
            continue
        quoted = re.findall(r'"([^"]*)"', line)
        if any(claim in q.lower() for q in quoted):
            continue
        return True
    return False


def rule_four(root: Path, problems: list[str]) -> None:
    """The front page does not deny code that exists."""
    front = (root / "README.md").read_text(encoding="utf-8")
    with_code = [p.name for p in project_dirs(root) if code_files(p)]
    for claim in NO_CODE_CLAIMS:
        if with_code and asserted(front, claim):
            problems.append(
                "README.md still says \"%s\" while %d project(s) carry code: %s."
                % (claim, len(with_code), ", ".join(with_code[:6])))


def main(argv: list[str]) -> int:
    if len(argv) > 2:
        print(__doc__.splitlines()[2].strip())
        return 2
    root = Path(argv[1]) if len(argv) == 2 else DEFAULT_ROOT
    if not (root / "README.md").exists():
        print("REFUSED: %s has no README.md, so there is no front page to check." % root)
        return 2

    problems: list[str] = []
    rule_one(root, problems)
    rule_two(root, problems)
    rule_three(root, problems)
    rule_four(root, problems)

    projects = project_dirs(root)
    pages = sum(len(prose_pages(p)) for p in projects)
    gates = workflow_gates(root)
    with_code = [p for p in projects if code_files(p)]
    print("%d project READMEs, %d prose pages under docs/, %d gates in the workflow, "
          "%d projects carrying code"
          % (len(projects), pages, len(gates), len(with_code)))

    if problems:
        print()
        print("REFUSED, %d problem(s):" % len(problems))
        for p in problems:
            print("  - %s" % p)
        print()
        print("Every one of these is a README that stopped describing its own directory. Fix the")
        print("README, not this script, unless the rule itself is wrong.")
        return 1

    print("every README names its own pages, claims no less and no more than the tree holds")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
