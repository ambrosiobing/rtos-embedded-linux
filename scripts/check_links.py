#!/usr/bin/env python3
"""Every relative link in the published Markdown resolves, and every anchor has
a heading.

    python scripts/check_links.py

Why this exists. The figures in this repository are not where the converter puts
them: `publish.py` moves each chapter's five into the project they belong to and
rewrites 101 image links to match. A rewrite that is wrong produces a page of
broken images, and nothing else here would notice. The linter reads prose and the
name scan reads words; neither looks at a link.

External links are counted and left alone: whether a vendor's page still exists
is not something a repository check can answer, and a network call would make
this fail for reasons that have nothing to do with the commit.

Exit status is non-zero if anything is unresolved, so CI can use it directly.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

INLINE = re.compile(r"\[[^\]]*\]\(\s*<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\s*\)")
REFERENCE = re.compile(r"^\s*\[[^\]]+\]:\s*<?(\S+)>?\s*$", re.MULTILINE)
FENCE = re.compile(r"^```.*?^```", re.S | re.M)
HEADING = re.compile(r"^#{1,6}\s+(.+?)\s*$", re.M)

PROBLEMS: list[str] = []


def slugify(heading: str) -> str:
    """GitHub's anchor rule, near enough: lower case, punctuation dropped,
    spaces to hyphens. Inline markup is stripped first."""
    h = re.sub(r"`([^`]*)`", r"\1", heading)
    h = re.sub(r"\*\*?([^*]*)\*\*?", r"\1", h)
    h = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", h)
    h = h.lower()
    h = re.sub(r"[^\w\s-]", "", h)
    return re.sub(r"\s+", "-", h.strip())


def anchors(path: Path) -> set[str]:
    text = FENCE.sub("\n", path.read_text(encoding="utf-8"))
    return {slugify(m.group(1)) for m in HEADING.finditer(text)}


def main() -> int:
    files = sorted(ROOT.rglob("*.md"))
    files = [f for f in files
             if not any(part in (".git", "build", ".venv", "__pycache__")
                        for part in f.relative_to(ROOT).parts)]
    if not files:
        print("no Markdown found, which cannot be right")
        return 1

    anchor_cache: dict[Path, set[str]] = {}
    checked = external = 0

    for path in files:
        rel = path.relative_to(ROOT).as_posix()
        text = FENCE.sub("\n", path.read_text(encoding="utf-8"))
        targets = [m.group(1) for m in INLINE.finditer(text)]
        targets += [m.group(1) for m in REFERENCE.finditer(text)]

        for target in targets:
            if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target) or target.startswith("//"):
                external += 1
                continue
            checked += 1

            frag = ""
            if "#" in target:
                target, frag = target.split("#", 1)

            if target:
                dest = (path.parent / target).resolve()
                if not dest.exists():
                    PROBLEMS.append("%s: names %s, which does not exist"
                                    % (rel, target))
                    continue
            else:
                dest = path          # a bare #fragment, in this file

            if frag:
                if dest.is_dir():
                    PROBLEMS.append("%s: %s#%s points a fragment at a directory"
                                    % (rel, target, frag))
                    continue
                if dest.suffix == ".md":
                    if dest not in anchor_cache:
                        anchor_cache[dest] = anchors(dest)
                    if frag.lower() not in anchor_cache[dest]:
                        PROBLEMS.append("%s: no heading for #%s in %s"
                                        % (rel, frag, target or dest.name))

    print("%d Markdown files, %d relative links checked, %d external links left alone"
          % (len(files), checked, external))
    if PROBLEMS:
        print("\n%d problem(s):" % len(PROBLEMS))
        for p in PROBLEMS:
            print("  " + p)
        return 1
    print("every relative link resolves and every anchor has a heading")
    return 0


if __name__ == "__main__":
    sys.exit(main())
