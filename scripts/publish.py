#!/usr/bin/env python3
"""Turn the Markdown converter's output into the shape this repository publishes.

    python mdbuild.py --out build/md
    python scripts/publish.py

`mdbuild.py` writes a central `chapters/` and a central `figures/`, and every
image link it emits reads `../figures/NAME.svg`. That shape is shared with the
sibling volumes and with `build.py --drift`, which proves the toolchain has not
been forked, so it is not edited. This script is the one place that knows the
published shape instead:

  * a chapter's five figures live with the project they belong to, under
    `projects/NN-slug/docs/figures/`, as the rendered SVG and nothing else. No
    LaTeX is published, figure sources included;
  * `front_stack.svg` belongs to no project, so it goes to `docs/figures/`;
  * every image link in every chapter is rewritten to point at wherever this
    script put the file;
  * typography that is right for a typeset page and wrong for a Markdown file is
    normalised, which is explained at NORMALISE below.

Why a figure goes to a project by its OWN name and not by the chapter that draws
it. `z09_arch.svg` belongs to project 09 whether chapter 9 is the only chapter
that shows it or not. Routing by the referencing chapter would put a figure in
two places the first time one chapter quoted another's diagram, and the duplicate
would then drift.

The project directory for chapter NN is derived from the chapter's own filename,
so the twenty names need no second list to fall out of step with: chapter
`09-fleet-update-and-the-fleet-shadow.md` publishes into
`projects/09-fleet-update-and-the-fleet-shadow/`.

Nothing is staged and nothing is committed. Exit status is non-zero if a figure a
chapter names is missing, so this cannot half-publish and report success.
"""
from __future__ import annotations

import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "build" / "md"
CHAPTERS = ROOT / "chapters"
PROJECTS = ROOT / "projects"
ROOT_FIGS = ROOT / "docs" / "figures"

# Figures come from the authoring directory, which holds the rendered SVG.
#
# Why not from build/md/figures, which is where mdbuild just wrote: because
# mdbuild copies each SVG out of the LaTeX figure-render scratch, so it carries
# only the figures a pdflatex run happened to leave there, and there is no TeX
# installation on the authoring laptop. The SVGs in figures/ are the rendered
# ones the volume has had all along, already carrying the white ground that keeps
# line art legible in a dark theme.
FIGURES = ROOT / "figures"

# ![caption](../figures/NAME.svg), which is the only image shape mdbuild writes.
LINK = re.compile(r"(!\[[^\]]*\]\()\.\./figures/([A-Za-z0-9_.-]+)\.svg(\))")

# zNN_something. The NN is the project the figure belongs to.
CHAPTER_FIG = re.compile(r"^z(\d\d)_")

# NN-slug.md, the chapter filename, which is also the project directory name.
CHAPTER_FILE = re.compile(r"^(\d\d)-(.+)\.md$")

# Typography the LaTeX converter is right to produce for a typeset page and
# wrong to produce for a published Markdown file.
#
# `build.py` maps `\,` to a thin space and `mdbuild.py` turns a trailing
# apostrophe into a right single quote. Both are correct in a PDF. Neither is
# correct here, and build.py in particular cannot be edited: it is byte-identical
# to its sibling volumes outside its DOC block, which `build.py --drift` proves,
# so the fix belongs at the published end rather than in the shared parser.
#
# Why these characters and not a general ban:
#
#   * the thin, no-break and em spaces are INVISIBLE. "3<thin>920 B" looks exactly
#     like "3 920 B" and matches neither "3920" nor "3 920" when a reader searches
#     the page, and a command copied out of a chapter carries a byte no shell
#     wants.
#   * the curly quotes are usually a plural possessive the converter rewrote. A
#     reader searching for the name does not find it, and the house rule is
#     straight quotes.
#
# Characters that carry meaning and are visible stay: micro, ohm, degree,
# multiplication, plus-minus.
NORMALISE = {
    " ": " ",   # thin space, from \,
    " ": " ",   # no-break space
    " ": " ",   # em space, from \quad
    "’": "'",   # right single quote
    "‘": "'",   # left single quote
    "“": '"',   # left double quote
    "”": '"',   # right double quote
}

PROBLEMS: list[str] = []


def chapter_projects() -> dict[str, str]:
    """number -> project directory name, read off the chapter filenames."""
    out = {}
    for src in sorted((SRC / "chapters").glob("*.md")):
        m = CHAPTER_FILE.match(src.name)
        if m and m.group(1) not in ("00", "21"):
            out[m.group(1)] = "%s-%s" % (m.group(1), m.group(2))
    return out


def normalise(text: str) -> tuple[str, int]:
    n = 0
    for bad, good in NORMALISE.items():
        if bad in text:
            n += text.count(bad)
            text = text.replace(bad, good)
    return text, n


def publish_figure(name: str, out_dir: Path) -> None:
    """The rendered SVG, and only that.

    The TikZ source is NOT published. It is part of the authoring half, like the
    chapter sources it sits beside, and this repository carries Markdown and the
    images that Markdown needs: no LaTeX of any kind. What a reader loses is the
    ability to see how a diagram is drawn, and that is the deliberate trade. What
    they gain is a tree with one markup language in it.
    """
    out_dir.mkdir(parents=True, exist_ok=True)
    svg = FIGURES / ("%s.svg" % name)
    if not svg.is_file():
        PROBLEMS.append("figure %s.svg is named by a chapter and is not in %s"
                        % (name, FIGURES.relative_to(ROOT).as_posix()))
        return
    shutil.copyfile(svg, out_dir / ("%s.svg" % name))


def main() -> int:
    if not (SRC / "chapters").is_dir():
        print("nothing to publish: %s does not exist.\n"
              "Run  python mdbuild.py --out build/md  first."
              % (SRC / "chapters").relative_to(ROOT).as_posix())
        return 1

    projects = chapter_projects()
    if len(projects) != 20:
        PROBLEMS.append("found %d numbered chapters, expected 20" % len(projects))

    CHAPTERS.mkdir(exist_ok=True)
    written, figures, fixed = 0, set(), 0

    for src in sorted((SRC / "chapters").glob("*.md")):
        text = src.read_text(encoding="utf-8")
        names: list[str] = []

        def rewrite(m: re.Match) -> str:
            name = m.group(2)
            fig = CHAPTER_FIG.match(name)
            if fig:
                nn = fig.group(1)
                if nn not in projects:
                    PROBLEMS.append("figure %s belongs to project %s, which has "
                                    "no chapter" % (name, nn))
                    return m.group(0)
                out_dir = PROJECTS / projects[nn] / "docs" / "figures"
                link = "../projects/%s/docs/figures" % projects[nn]
            else:
                # front_stack and anything else with no chapter number.
                out_dir, link = ROOT_FIGS, "../docs/figures"

            names.append(name)
            if name not in figures:
                publish_figure(name, out_dir)
                figures.add(name)
            return "%s%s/%s.svg%s" % (m.group(1), link, name, m.group(3))

        text = LINK.sub(rewrite, text)
        text, n = normalise(text)
        fixed += n
        (CHAPTERS / src.name).write_text(text, encoding="utf-8")
        written += 1
        print("  chapters/%-52s %2d figures%s"
              % (src.name, len(names),
                 ", %d characters normalised" % n if n else ""))

    print("\n%d chapters, %d figures, %d characters normalised"
          % (written, len(figures), fixed))

    if PROBLEMS:
        print("\n%d problem(s):" % len(PROBLEMS))
        for p in PROBLEMS:
            print("  " + p)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
