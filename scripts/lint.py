#!/usr/bin/env python3
"""House-style linter over the published Markdown, including the name scan.

    python scripts/lint.py                     every published Markdown file
    python scripts/lint.py chapters/07-the-zones.md

Checks prose (everything outside fenced code) for: em and en dashes and the
LaTeX -- / --- that survive a conversion, non-ASCII characters, violent idioms,
and the names this volume does not print. Checks code blocks for non-ASCII and
for lines longer than the page can print.

This is the published-side copy. The root lint.py is the same checks over the
section sources, and it stays on the authoring machine with them. Both exist
because the name scan has to run on what was published, not only on what was
written: a forbidden token that survived the conversion would be in the public
tree, and only this copy can see that.

Two scans deliberately do NOT spell what they look for.

The first is the tooling-attribution scan, which is not here at all. A rule that
spells the words it forbids would put those words into the repository it
protects, which is the opposite of what it is for, so that scan lives outside
this repository and is run by hand before anything is published.

The second is the name scan below. This volume was written with one advert open
and must serve any application, so it prints no employer, no product, no town
and no region. Writing that list in plain text here would put the list in the
published tree, so each token is stored as the SHA-256 of its lower-case form
and every word of the prose is hashed and compared. The plaintext list lives
once, in the private catalogue note, and never in this repository.
"""
import hashlib
import re
import sys
from pathlib import Path

# One directory up, because this copy lives in scripts/ rather than at the root.
ROOT = Path(__file__).resolve().parent.parent

VERB = re.compile(r"\\begin\{(asciiart|ccode|cppcode|shellcode|pycode|makecode|dtscode"
                  r"|yamlcode|plaincode|lstlisting|verbatim)\}"
                  r"(.*?)\\end\{\1\}", re.S)
FENCE = re.compile(r"^```.*?^```", re.S | re.M)
# In the generated Markdown edition a run of hyphens is table or rule syntax,
# not a dash in prose. These two forms are removed before the dash check so
# that the check keeps meaning something.
MD_TABLE_RULE = re.compile(r"^\s*\|?[\s:|-]*-[\s:|-]*\|?\s*$", re.M)
MD_HRULE = re.compile(r"^\s*-{3,}\s*$", re.M)
# An inline code span is a command, not prose: a command-line double dash in
# one is correct and must not be reported as a dash.
MD_INLINE_CODE = re.compile(r"`[^`\n]+`")
DASH = re.compile(r"(.{0,40})(\u2014|\u2013|(?<![-\w])---?(?![-\w>]))(.{0,40})")
VIOLENT = re.compile(r"\b(kill(?:s|ed|ing)?|attack(?:s|ed|ing)?|fight(?:s|ing)?|blame[ds]?"
                     r"|hurt(?:s|ing)?|destroy(?:s|ed|ing)?|suicid\w*|war against)\b", re.I)

# The subsection skeleton every chapter carries, in this order.
REQUIRED = ["Why this project", "Prior art and what to reuse",
            "Parts from the inventory", "System architecture", "Configuration",
            "Wiring", "Memory and timing budget", "Software design",
            "Data flow", "Repository layout", "Steps",
            "Build, flash and debug", "Verification and acceptance criteria",
            "Variants", "Pitfalls", "Best practices applied", "Stretch goals",
            "Roadmap and next steps", "Portfolio evidence", "Sources"]
FIGURES = ("arch", "wiring", "uml", "mem", "timing")

MAXLEN = {"asciiart": 112}   # scriptsize fits ~115 columns
MAXCODE = 99                 # footnotesize at basewidth 0.48em fits ~99 columns

# SHA-256 of each forbidden token, lower case. One-word tokens are matched
# against single words; the two-word tokens against adjacent pairs.
FORBIDDEN_1 = {
    "3ee41cff3be66189d97719289a9aa656ab744b0e1927c355382fdc94d5251a96",
    "1b0dc93c85a2f74b5ca592a4a907d1eec7ac75e70eb263a466b32a9133234b48",
    "d3d82dcb8b377cf07eb2e2060ee88393f25311095ce1743ce4bcc3c13c4a85c2",
    "be75dce359819a9b2f87c252acd03e8d3ea3bc9140b8fad6c124481d25e507cb",
    "425c89ed5bb78a7623fae60fd8a6f648488168740fe82cf6ad34caa4d07aa972",
    "049c287ed3e2d554fabbbf4055dc3621a8bf44852f372b29a1be7570653fe789",
}
FORBIDDEN_2 = {
    "33e543b5243eb422379a938de860c455e791b6e297bc781c72c164154daab16a",
    "855877636ad2d172ad38de350edbe018692a63e321e527bc36e5ff8296cddfdc",
    "bdbedc5d6acf46cb6d5434a64678600739b68b44944619a5611e88533cbc9fcc",
}
WORD = re.compile(r"[A-Za-z][A-Za-z0-9'-]*")


def _h(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def name_scan(text):
    """Report a forbidden name without ever printing the list it checks.

    The whole file is scanned, code blocks included: a name is as wrong in a
    hostname, a topic string or a comment as it is in a sentence.
    """
    problems = []
    words = [(m.group(0).lower(), m.start()) for m in WORD.finditer(text)]
    for i, (w, pos) in enumerate(words):
        if _h(w) in FORBIDDEN_1:
            line = text.count("\n", 0, pos) + 1
            problems.append(f"line {line}: a name this volume does not print. "
                            f"It begins {w[0]!r} and is {len(w)} letters long. "
                            f"Say what the thing is, not whose it is")
        if i + 1 < len(words):
            pair = w + " " + words[i + 1][0]
            if _h(pair) in FORBIDDEN_2:
                line = text.count("\n", 0, pos) + 1
                problems.append(f"line {line}: a two-word name this volume does "
                                f"not print, beginning {w[0]!r}. Use the generic noun")
    return problems


def check(path):
    s = path.read_text(encoding="utf-8")
    problems = []
    is_md = path.suffix == ".md"

    if is_md:
        codes = [("fence", m.group(0)) for m in FENCE.finditer(s)]
        prose = FENCE.sub("\n", s)
        prose = MD_HRULE.sub("\n", prose)
        prose = MD_TABLE_RULE.sub("\n", prose)
        prose = MD_INLINE_CODE.sub(" ", prose)
    else:
        codes = [(m.group(1), m.group(2)) for m in VERB.finditer(s)]
        prose = VERB.sub("\n", s)

    for m in DASH.finditer(prose):
        problems.append(f"dash: ...{m.group(1)}[{m.group(2)}]{m.group(3)}...".replace("\n", " "))
    bad = sorted({c for c in prose if ord(c) > 126})
    if bad:
        problems.append(f"non-ASCII in prose: {bad}")
    for m in VIOLENT.finditer(prose):
        problems.append(f"violent idiom: {m.group(0)!r} near "
                        f"{prose[max(0, m.start()-40):m.end()+40]!r}")

    problems += name_scan(s)

    for env, code in codes:
        bad = sorted({c for c in code if ord(c) > 126})
        if bad:
            problems.append(f"non-ASCII in {env} block: {bad}")
        limit = MAXLEN.get(env, MAXCODE)
        for ln in code.splitlines():
            if len(ln) > limit:
                problems.append(f"{env} line {len(ln)} chars (max {limit}): {ln[:50]}...")

    if re.fullmatch(r"z\d\d", path.stem):
        for h in REQUIRED:
            if h not in s:
                problems.append(f"missing subsection: {h}")
        for fig in FIGURES:
            if f"{{{path.stem}_{fig}}}" not in s:
                problems.append(f"missing figure: {path.stem}_{fig}")
        # Every budget table carries a Measured column, and a chapter that has
        # not been to the bench says so in words rather than leaving a blank.
        if "Measured" not in s:
            problems.append("budget table has no Measured column")
        # The evidence box names one idiom and the command that proves it. A
        # chapter that lists every idiom has named none.
        m = re.search(r"\\subsection\*\{Portfolio evidence\}(.*?)(?:\\subsection\*|\Z)",
                      s, re.S)
        if m and "texttt" not in m.group(1):
            problems.append("Portfolio evidence names no command")
    return problems


def published_files():
    """Everything a clone can read, which is what this copy exists to check.

    The section sources are not here: they are the authoring half and stay on the
    authoring machine, so the root lint.py checks those and this one checks what
    was published from them. The name scan is the reason both exist rather than
    one: a forbidden token that survived the conversion would be in the public
    tree, and only this list can see that.
    """
    files = sorted((ROOT / "chapters").glob("*.md"))
    files += sorted((ROOT / "projects").rglob("*.md"))
    for name in ("README.md", "AUTHORING.md", "SOURCE.md", "CONTENTS.md"):
        p = ROOT / name
        if p.is_file():
            files.append(p)
    return files


def main(argv):
    files = [Path(a) if Path(a).is_absolute() else ROOT / a for a in argv] or \
            published_files()
    total = 0
    for f in files:
        pr = check(f)
        total += len(pr)
        if pr:
            print(f"== {f.name}: {len(pr)} problems")
            for p in pr[:25]:
                print("   " + p)
            if len(pr) > 25:
                print(f"   ... and {len(pr)-25} more")
        else:
            print(f"== {f.name}: clean")
    print("TOTAL PROBLEMS:", total)
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
