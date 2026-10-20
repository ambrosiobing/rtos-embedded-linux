#!/usr/bin/env python3
"""The refactor's rule, as a gate: a chapter is anchored or it is not, and the tree says which.

    python scripts/check_anchors.py

Written on Tuesday 20 October 2026 as step two of docs/REFACTOR_PLAN.md. Every chapter is being
brought to the same shape as the first four already have: its prior-art table carries one row
whose first cell begins "The anchor", naming the published paper, thesis, standard, documented
figure or repository the chapter replicates, and its acceptance criteria carry one item beginning
"The replicated claim", stating the number or behaviour the anchor claims and, in the same item,
the figure that would refute it.

The rule is written before the chapters are brought level, so that each chapter is checked
against the rule and not against memory of the plan. Two things keep that honest:

  1. ANCHORED below lists the chapters that have been brought level. A listed chapter missing
     either half fails. An unlisted chapter is not required to have them, so the tree stays green
     while the work proceeds one chapter per commit.
  2. The list cannot drift from the tree. An unlisted chapter that carries either half fails
     too, with the instruction to add it, so a chapter is counted the commit it is anchored and
     the count printed at the end is the count in the tree, not the count somebody remembered.

The replicated claim must carry its refutation in the same item, because a claim without a number
it can miss is not a replication. That is the sentence the plan is built on and it is enforced
here rather than reread.

Shown to refuse before it was committed: scripts/test_check_anchors.py builds synthetic chapters
and mutates them seven ways.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Chapter numbers brought level with the rule, in the order the plan gives. A chapter joins this
# set in the same commit that gives it its two halves, and in no other.
ANCHORED = {4, 5, 6, 7, 8}

PRIOR_ART = "## Prior art and what to reuse"
CRITERIA = "## Verification and acceptance criteria"
ANCHOR_CELL = "The anchor"
CLAIM_ITEM = "The replicated claim"
REFUTED = "*Refuted if*"


def section(text, heading):
    """The body of one level-two section, or None when the chapter has no such heading."""
    start = text.find(heading + "\n")
    if start < 0:
        return None
    body = text[start + len(heading):]
    nxt = re.search(r"^## ", body, re.M)
    return body[:nxt.start()] if nxt else body


def plain(cell):
    """A table cell or list item with its Markdown emphasis removed, for the prefix test."""
    return re.sub(r"[*_`]", "", cell).strip()


def anchor_rows(prior_art):
    rows = []
    for line in prior_art.splitlines():
        if not line.lstrip().startswith("|"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if not cells or set(cells[0]) <= set(":- "):
            continue                       # the header rule
        if plain(cells[0]).startswith(ANCHOR_CELL):
            rows.append(cells)
    return rows


def claim_items(criteria):
    items = []
    for line in criteria.splitlines():
        if not line.startswith("- "):
            continue
        if plain(line[2:]).startswith(CLAIM_ITEM):
            items.append(line[2:])
    return items


def check_chapter(path, number, anchored):
    text = path.read_text(encoding="utf-8")
    problems = []
    prior = section(text, PRIOR_ART)
    crit = section(text, CRITERIA)
    if prior is None:
        problems.append("no section %r" % PRIOR_ART)
    if crit is None:
        problems.append("no section %r" % CRITERIA)
    if problems:
        return problems, False

    rows = anchor_rows(prior)
    items = claim_items(crit)
    listed = number in anchored

    if listed:
        if len(rows) != 1:
            problems.append("listed as anchored but its prior-art table carries %d rows "
                            "beginning %r, want exactly 1" % (len(rows), ANCHOR_CELL))
        elif any(not c for c in rows[0]) or len(rows[0]) < 4:
            problems.append("the anchor row leaves a cell empty; source, what it gives, what "
                            "it does not and the licence are all required")
        if len(items) != 1:
            problems.append("listed as anchored but its acceptance criteria carry %d items "
                            "beginning %r, want exactly 1" % (len(items), CLAIM_ITEM))
        elif REFUTED not in items[0]:
            problems.append("the replicated claim names no refutation: the item must carry "
                            "%r, with the figure that would refute it, in the same item"
                            % REFUTED)
    else:
        if rows or items:
            problems.append("carries %d anchor row(s) and %d replicated-claim item(s) but is "
                            "not listed in ANCHORED; a chapter joins the list in the commit "
                            "that anchors it" % (len(rows), len(items)))
    return problems, listed and not problems


def check_tree(root, anchored):
    """Every numbered chapter under root/chapters. Returns (problems by file, anchored count)."""
    found = {}
    count = 0
    for n in range(1, 21):
        matches = sorted((root / "chapters").glob("%02d-*.md" % n))
        if len(matches) != 1:
            found["chapters/%02d-*.md" % n] = ["matches %d files, want 1" % len(matches)]
            continue
        problems, ok = check_chapter(matches[0], n, anchored)
        if problems:
            found[matches[0].name] = problems
        if ok:
            count += 1
    for n in sorted(anchored):
        if not (1 <= n <= 20):
            found["ANCHORED"] = ["lists chapter %d, which does not exist" % n]
    return found, count


def main():
    found, count = check_tree(ROOT, ANCHORED)
    for name in sorted(found):
        print("== %s" % name)
        for p in found[name]:
            print("   " + p)
    print("%d of 20 chapters anchored, %d listed" % (count, len(ANCHORED)))
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
