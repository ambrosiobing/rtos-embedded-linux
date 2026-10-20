#!/usr/bin/env python3
"""The anchor gate, shown to refuse before it is trusted.

    python scripts/test_check_anchors.py

Builds a synthetic twenty-chapter tree in a temporary directory, with the two sections the gate
reads, and mutates it seven ways. Each mutation must be refused for the reason the gate states,
and the two clean shapes must pass. The production list in check_anchors.py is not touched: the
functions take the anchored set as an argument, so nothing here can move the expectation along
with the work.
"""
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_anchors as ca  # noqa: E402

SLUGS = ["presence", "the-claim", "one-sensor", "kernel", "zones"] + ["chapter-%d" % n
                                                                     for n in range(6, 21)]
ANCHOR_ROW = ("| The anchor: a published comparison | a context switch of 223 against 524 cycles "
              "| numbers for this board | Published |\n")
CLAIM_ITEM = ("- **The replicated claim: the ratio between the kernels. Open.** Here it is 1.10 "
              "against 2.35. *Refuted if* the ratio contradicts theirs without a stated reason.\n")


def chapter(anchored, row=ANCHOR_ROW, item=CLAIM_ITEM, extra_rows=0, extra_items=0):
    rows = "| Source | What it gives | What it does not | Licence |\n| --- | --- | --- | --- |\n"
    rows += "| The vendor's driver | the ranging | the binding | Vendor |\n"
    if anchored:
        rows += row * (1 + extra_rows)
    items = "- **The device comes up.** *Refuted if* it does not.\n"
    if anchored:
        items += item * (1 + extra_items)
    return ("# A chapter\n\n## Why this project\n\nText.\n\n## Prior art and what to reuse\n\n"
            + rows + "\n*Table. Prior art.*\n\n## Parts from the inventory\n\nText.\n\n"
            "## Verification and acceptance criteria\n\n" + items + "\n## Variants\n\nText.\n")


def tree(root, anchored, **kw):
    (root / "chapters").mkdir(exist_ok=True)
    for n, slug in enumerate(SLUGS, start=1):
        (root / "chapters" / ("%02d-%s.md" % (n, slug))).write_text(
            chapter(n in anchored, **kw), encoding="utf-8")


def expect(name, found, count, want_count, *needles):
    """Every needle must appear in some problem, or, with no needles, there must be none."""
    text = "\n".join("%s: %s" % (k, p) for k, ps in found.items() for p in ps)
    ok = count == want_count and (
        (not needles and not found) or all(n in text for n in needles))
    print("%s %s" % ("PASS" if ok else "FAIL", name))
    if not ok:
        print("   count %d want %d; problems:\n%s" % (count, want_count, text or "   none"))
    return ok


def main():
    results = []
    with tempfile.TemporaryDirectory() as d:
        root = Path(d)

        tree(root, set())
        found, count = ca.check_tree(root, set())
        results.append(expect("clean tree, nothing anchored, passes with 0", found, count, 0))

        tree(root, {4, 5})
        found, count = ca.check_tree(root, {4, 5})
        results.append(expect("two anchored chapters, both listed, passes with 2",
                              found, count, 2))

        # 1. Listed but the row is missing.
        tree(root, {4, 5})
        p = next((root / "chapters").glob("04-*.md"))
        p.write_text(p.read_text(encoding="utf-8").replace(ANCHOR_ROW, ""), encoding="utf-8")
        found, count = ca.check_tree(root, {4, 5})
        results.append(expect("listed chapter missing its anchor row is refused",
                              found, count, 1, "carries 0 rows beginning 'The anchor'"))

        # 2. Listed but the item is missing.
        tree(root, {4, 5})
        p = next((root / "chapters").glob("05-*.md"))
        p.write_text(p.read_text(encoding="utf-8").replace(CLAIM_ITEM, ""), encoding="utf-8")
        found, count = ca.check_tree(root, {4, 5})
        results.append(expect("listed chapter missing its replicated claim is refused",
                              found, count, 1, "carry 0 items beginning 'The replicated claim'"))

        # 3. The claim names no refutation.
        tree(root, {4, 5})
        p = next((root / "chapters").glob("04-*.md"))
        p.write_text(p.read_text(encoding="utf-8").replace("*Refuted if* the ratio", "If the ratio"),
                     encoding="utf-8")
        found, count = ca.check_tree(root, {4, 5})
        results.append(expect("a replicated claim with no refutation is refused",
                              found, count, 1, "names no refutation"))

        # 4. An unlisted chapter carries the row: the list has drifted from the tree.
        tree(root, {4, 5})
        found, count = ca.check_tree(root, {4})
        results.append(expect("an anchored chapter missing from the list is refused",
                              found, count, 1, "not listed in ANCHORED"))

        # 5. Two anchor rows in one chapter.
        tree(root, {4}, extra_rows=1)
        found, count = ca.check_tree(root, {4})
        results.append(expect("two anchor rows are refused", found, count, 0,
                              "carries 2 rows beginning 'The anchor'"))

        # 6. The anchor row leaves a cell empty.
        tree(root, {4}, row="| The anchor: a thing |  | numbers for this board | Published |\n")
        found, count = ca.check_tree(root, {4})
        results.append(expect("an anchor row with an empty cell is refused", found, count, 0,
                              "leaves a cell empty"))

        # 7. The list names a chapter that does not exist.
        tree(root, {4})
        found, count = ca.check_tree(root, {4, 21})
        results.append(expect("a listed chapter that does not exist is refused", found, count, 1,
                              "does not exist"))

    n = sum(1 for r in results if r)
    print("%d of %d checks passed" % (n, len(results)))
    return 0 if n == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
