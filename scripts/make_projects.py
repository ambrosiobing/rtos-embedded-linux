#!/usr/bin/env python3
"""One directory per project, with a README that leads with its state.

    python scripts/make_projects.py

All twenty exist from the start, so the shape of the volume is visible in the
repository rather than only in the book. Today every one of them says the same
thing, which is that nothing has been built: this volume is twenty written
chapters and no code. A directory that exists and says so is honest; a directory
that is missing lets a reader assume it is coming, and a README that implies work
has started would be a claim with nothing behind it.

Every field is read out of `sections/zNN.tex`, never invented here: the title and
target from the `\\project` line, and the board, peripherals, toolchain, operating
system, difficulty, effort and deliverable from the chapter's own key facts. So a
stub cannot disagree with its chapter, and correcting a chapter corrects the stub
on the next run.

**An existing README is never overwritten.** The moment one of these projects is
started, its README stops being a stub and becomes the project's own record, and
this script must not be able to undo that. It reports what it skipped.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SECTIONS = ROOT / "sections"
CHAPTERS = ROOT / "chapters"
PROJECTS = ROOT / "projects"

PROJECT_RE = re.compile(r"\\project\{(\d+)\}\{(.+?)\}\{(.+?)\}\{(.+?)\}", re.S)
KEYFACT_RE = re.compile(r"\\item\[([^\]]+)\]\s*(.+?)(?=\n\\item\[|\n\\end\{keyfacts\})",
                        re.S)
CHAPTER_FILE = re.compile(r"^(\d\d)-(.+)\.md$")


def detex(s: str) -> str:
    """Enough of the markup removed for a README line. The sources are prose."""
    s = re.sub(r"\\(?:textbf|textit|emph|texttt)\{([^{}]*)\}", r"\1", s)
    s = s.replace("\\,", " ").replace("\\&", "&").replace("\\%", "%")
    s = s.replace("~", " ").replace("--", "-")
    return re.sub(r"\s+", " ", s).strip()


def chapter_slugs() -> dict[str, str]:
    out = {}
    for p in sorted(CHAPTERS.glob("*.md")):
        m = CHAPTER_FILE.match(p.name)
        if m and m.group(1) not in ("00", "21"):
            out[m.group(1)] = "%s-%s" % (m.group(1), m.group(2))
    return out


def main() -> int:
    slugs = chapter_slugs()
    if len(slugs) != 20:
        print("found %d numbered chapters, expected 20. Run mdbuild and publish "
              "first." % len(slugs))
        return 1

    made, skipped = [], []
    for nn, slug in sorted(slugs.items()):
        tex = SECTIONS / ("z%s.tex" % nn)
        if not tex.is_file():
            print("missing %s" % tex.relative_to(ROOT).as_posix())
            return 1
        text = tex.read_text(encoding="utf-8")

        m = PROJECT_RE.search(text)
        if not m:
            print("no \\project line in %s" % tex.name)
            return 1
        title, target = detex(m.group(2)), detex(m.group(3))

        facts = {k.strip(): detex(v) for k, v in KEYFACT_RE.findall(text)}

        directory = PROJECTS / slug
        directory.mkdir(parents=True, exist_ok=True)
        readme = directory / "README.md"
        if readme.exists():
            skipped.append(slug)
            continue

        rows = []
        for label in ("Board", "Boards", "Peripherals", "Toolchain",
                      "Operating system", "Difficulty", "Effort"):
            if label in facts:
                rows.append("| %s | %s |" % (label, facts[label]))

        lines = [
            "# P%s. %s" % (nn, title),
            "",
            "Status: not started. Nothing in this directory has been built, and no",
            "number here has been measured.",
            "",
            "The written design is [chapter %s](../../chapters/%s.md), which is"
            % (nn, slug),
            "complete. What is absent is the code.",
            "",
            "| | |",
            "|---|---|",
            "| Target | %s |" % target,
        ] + rows + [
            "",
            "## What it would deliver",
            "",
            facts.get("Deliverable", "See the chapter."),
            "",
            "## Figures",
            "",
            "The chapter's five figures are in [docs/figures](docs/figures), as rendered",
            "SVG. The LaTeX that draws them stays on the authoring machine: this repository",
            "publishes no `.tex`, and `checks.yml` fails if one is ever committed.",
        ]
        readme.write_text("\n".join(lines) + "\n", encoding="utf-8")
        made.append(slug)

    print("%d README files written" % len(made))
    if skipped:
        print("%d left alone because they already exist: %s"
              % (len(skipped), ", ".join(skipped)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
