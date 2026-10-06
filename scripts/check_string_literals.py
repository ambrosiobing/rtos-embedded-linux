#!/usr/bin/env python3
"""Find a string literal left unterminated on its line, which no compiler here can.

A scripted edit passing through a shell heredoc loses one level of backslash, so an
intended newline escape inside a C, C++ or Rust string literal can arrive as a real
newline and split the literal across two lines. A compiler says "missing terminating
double quote" at once. The authoring laptop runs no compiler for this volume, so this is
the stand-in, and it has caught that damage twice.

    python scripts/check_string_literals.py                  # every source in the repo
    python scripts/check_string_literals.py projects/02-the-claim

THE RULE. A line with an odd number of unescaped double quotes is legitimate only when it
opens a continuation, meaning it ends with a backslash, or closes one begun on the line
before. Anything else is reported with its line number.

WHY IT TAKES PATHS ON THE COMMAND LINE, which is not a detail. An earlier version of this
file lived in a scratch directory with its root hardcoded to one project and ignored its
arguments entirely. It was run three times against a second project, reported "0 problems"
each time, and had not read a line of it. A check whose scope is invisible is worse than no
check, because its zero is mistaken for evidence. So the count of files examined is printed
with every run, and the paths are whatever was asked for.
"""

import sys
from pathlib import Path

SUFFIXES = (".c", ".h", ".cpp", ".hpp", ".rs")
SKIP_PARTS = ("target", ".git", "build", "build-host", "build-zephyr")

BACKSLASH = chr(92)
QUOTE = chr(34)
STAR = chr(42)
SLASH = chr(47)


def unescaped_quotes(line):
    count = 0
    i = 0
    while i < len(line):
        ch = line[i]
        if ch == BACKSLASH:
            i += 2
            continue
        if ch == QUOTE:
            count += 1
        i += 1
    return count


def sources(roots):
    for root in roots:
        if root.is_file():
            if root.suffix in SUFFIXES:
                yield root
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in SUFFIXES or not path.is_file():
                continue
            if any(part in SKIP_PARTS for part in path.parts):
                continue
            yield path


def check(path, repo):
    problems = 0
    continued = False

    for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        stripped = line.rstrip()
        lead = stripped.lstrip()

        # A comment carries prose, and prose carries quotation marks. Only code is
        # checked, which is where a split literal is a syntax error.
        if lead.startswith((STAR, SLASH + SLASH, SLASH + STAR)):
            continued = False
            continue

        odd = unescaped_quotes(stripped) % 2 == 1
        opens_continuation = stripped.endswith(BACKSLASH)

        if odd and not (opens_continuation or continued):
            try:
                shown = path.relative_to(repo)
            except ValueError:
                shown = path
            print("%s:%d  a string literal is not closed on this line" % (shown, n))
            print("    %s" % stripped[:88])
            problems += 1

        continued = opens_continuation and odd

    return problems


def main(argv):
    repo = Path(__file__).resolve().parent.parent
    roots = [Path(a).resolve() for a in argv[1:]] or [repo]

    problems = 0
    checked = 0
    for path in sources(roots):
        checked += 1
        problems += check(path, repo)

    names = []
    for r in roots:
        if r == repo:
            names.append("the repository")
            continue
        try:
            names.append(str(r.relative_to(repo)))
        except ValueError:
            # A path outside the repository is legitimate: the mutation runs that prove
            # this check can fail operate on copies in a scratch directory.
            names.append(str(r))

    print("%d source file(s) checked under %s, %d problem(s)"
          % (checked, ", ".join(names), problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
