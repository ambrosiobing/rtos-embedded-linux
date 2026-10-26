#!/usr/bin/env python3
"""Does the build stamp actually regenerate at every build? Proved, not assumed.

    python scripts/test_build_stamp.py                 the whole thing, needs cmake and ninja
    python scripts/test_build_stamp.py --static-only   the half that needs only Python

WHY THIS EXISTS. scripts/stamp.py replaced `__DATE__` so that a stale flash could be told from a
reproduction, and projects/05-the-zones/CMakeLists.txt wired it with a custom target carrying no
output, on the reasoning that such a target is always out of date and so a command depending on
one is never skipped. That reasoning is about make. Under ninja, cmake turns a target named in
DEPENDS into a dependency on that target's phony edge, and a phony edge with no inputs is already
up to date, so the stamp was written once when the build directory was new and never again.

WHAT IT COST. On Monday 26 October 2026 four captures were taken at the bench for the two
orientation runs of projects/05-the-zones/docs/ORIENTATION.md. All four printed `built Tuesday 20
October 2026, 11:57:44` and `from f2cdf51` while the tree was at 50ffade, so all four were read
as a stale flash. The flash was good, the build was good, and the stamp was eleven hours stale
because the build directory had been created on Tuesday. A guard that cries stale about a correct
image discards real results, which is the direction scripts/stamp.py's own docstring calls the
worse of the two.

AND THE FIRST FIX WAS ALSO WRONG, which is why this file runs four wirings rather than asserting
one. A rule whose output does not exist after it runs is widely held to be unconditional. Under
ninja it is not. Declaring that output SYMBOLIC is what makes the edge stay dirty, and the only
way to know which of those two sentences is true on a given generator is to build twice and look.

THE TWO HALVES ANSWER DIFFERENT QUESTIONS and neither is enough alone:

    static    the project is wired the way this file proves to work, and not the way it did
    dynamic   that wiring regenerates the file at every build, and the two that do not are
              shown not to, so a pass here is not a pass of an experiment that cannot fail
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
P05 = ROOT / "projects" / "05-the-zones" / "CMakeLists.txt"

GEN = 'import sys, time\nopen(sys.argv[1], "w").write("nonce %d\\n" % time.time_ns())\n'

HEAD = """cmake_minimum_required(VERSION 3.20)
project(stamp_fixture NONE)
set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/stamp.txt")
set(GENCMD "${PY}" "${CMAKE_CURRENT_SOURCE_DIR}/gen.py" "${STAMP}")
"""

# The four wirings, and what each must do. A wiring expected to stamp once is as much a part of
# the test as one expected to regenerate: it is what makes a pass mean something.
WIRINGS = {
    "target_in_depends": ("""
add_custom_target(stamp_always)
add_custom_command(OUTPUT "${STAMP}" COMMAND ${GENCMD} DEPENDS stamp_always VERBATIM)
add_custom_target(consume ALL DEPENDS "${STAMP}")
""", False, "what P05 shipped until Monday 26 October 2026"),

    "output_never_made": ("""
set(NEVER "${CMAKE_CURRENT_BINARY_DIR}/stamp.never-made")
add_custom_command(OUTPUT "${NEVER}" COMMAND "${CMAKE_COMMAND}" -E true VERBATIM)
add_custom_command(OUTPUT "${STAMP}" COMMAND ${GENCMD} DEPENDS "${NEVER}" VERBATIM)
add_custom_target(consume ALL DEPENDS "${STAMP}")
""", False, "the obvious fix, and it does not work under this generator"),

    "output_never_made_symbolic": ("""
set(NEVER "${CMAKE_CURRENT_BINARY_DIR}/stamp.never-made")
add_custom_command(OUTPUT "${NEVER}" COMMAND "${CMAKE_COMMAND}" -E true VERBATIM)
set_source_files_properties("${NEVER}" PROPERTIES SYMBOLIC TRUE)
add_custom_command(OUTPUT "${STAMP}" COMMAND ${GENCMD} DEPENDS "${NEVER}" VERBATIM)
add_custom_target(consume ALL DEPENDS "${STAMP}")
""", True, "what P05 uses now, and the one real file is what the compile depends on"),

    "custom_target_command": ("""
add_custom_target(stamp_now ALL COMMAND ${GENCMD} BYPRODUCTS "${STAMP}" VERBATIM)
add_custom_target(consume ALL)
add_dependencies(consume stamp_now)
""", True, "also works, and reaches the compile by target ordering rather than by the file"),
}


def code_only(text: str) -> str:
    """The file with its comment lines removed, which is the only form worth searching.

    THE FIRST VERSION OF THIS FILE SEARCHED THE WHOLE TEXT and refused on its own account: the
    wiring that was removed is quoted in a comment in that CMakeLists, as the record of what went
    wrong, so a search for it finds the record and calls it a defect. That is the fourth time in
    this repository that a checker has matched prose it or its subject wrote about itself, and
    the fix is the same every time, which is to look only where the thing being checked can
    actually take effect.
    """
    return "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("#"))


def static_checks() -> list[str]:
    """The project is wired the way the dynamic half proves to work."""
    problems = []
    text = P05.read_text(encoding="utf-8")
    code = code_only(text)

    if "add_custom_target(stamp_always)" in code:
        problems.append(
            "%s still declares add_custom_target(stamp_always) as a live command. That wiring "
            "stamps once per build directory and never again." % P05.relative_to(ROOT))

    if "DEPENDS stamp_always" in code:
        problems.append(
            "%s names stamp_always in a live DEPENDS." % P05.relative_to(ROOT))

    if "SYMBOLIC TRUE" not in code:
        problems.append(
            "%s does not declare its never-made stamp dependency SYMBOLIC. Without that property "
            "the rule is skipped from the second build onward, which is this file's whole "
            "subject." % P05.relative_to(ROOT))

    if "stamp.py" not in code:
        problems.append("%s no longer runs scripts/stamp.py at all." % P05.relative_to(ROOT))

    # AND THE RECORD OF THE DEFECT IS ITSELF CHECKED, because a later reader tidying the comment
    # away would remove the only explanation of why that property is there, and the property on
    # its own looks like something safe to drop.
    if "stamp_always" not in text:
        problems.append(
            "%s no longer mentions the wiring that failed, not even in a comment. The SYMBOLIC "
            "property above it is unexplained without it and reads as removable."
            % P05.relative_to(ROOT))

    return problems


def build_twice(tools: dict, name: str, body: str, work: Path) -> tuple[str, str]:
    """Configure once, build twice, and return the stamp as it stood after each build."""
    src, build = work / name, work / ("build-" + name)
    src.mkdir(parents=True)
    (src / "gen.py").write_text(GEN, encoding="utf-8")
    (src / "CMakeLists.txt").write_text(HEAD + body, encoding="utf-8")

    def run(args):
        r = subprocess.run([str(a) for a in args], cwd=str(work),
                           capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError("%s failed for wiring %s:\n%s\n%s"
                               % (args[0], name, r.stdout[-1200:], r.stderr[-1200:]))

    run([tools["cmake"], "-S", src, "-B", build, "-G", "Ninja",
         "-DCMAKE_MAKE_PROGRAM=" + str(tools["ninja"]), "-DPY=" + sys.executable])
    run([tools["ninja"], "-C", build])
    stamp = build / "stamp.txt"
    first = stamp.read_text(encoding="utf-8").strip() if stamp.exists() else ""
    run([tools["ninja"], "-C", build])
    second = stamp.read_text(encoding="utf-8").strip() if stamp.exists() else ""
    return first, second


def dynamic_checks(tools: dict) -> list[str]:
    problems = []
    with tempfile.TemporaryDirectory(prefix="stampgate") as tmp:
        work = Path(tmp)
        for name, (body, must_regenerate, note) in WIRINGS.items():
            first, second = build_twice(tools, name, body, work)
            if not first:
                problems.append("%s wrote no stamp on the first build at all" % name)
                continue
            regenerated = first != second
            verdict = "regenerates" if regenerated else "stamps once only"
            print("  %-26s %-18s %s" % (name, verdict, note))
            if regenerated != must_regenerate:
                problems.append(
                    "%s was expected to %s and it does not. Either this generator behaves "
                    "differently than when the project was wired, or the wiring in this file no "
                    "longer matches the project's."
                    % (name, "regenerate every build" if must_regenerate
                       else "stamp once only, which is what makes this test able to fail"))
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--static-only", action="store_true",
                        help="skip the half that needs cmake and ninja, and say so loudly")
    args = parser.parse_args()
    built = False

    print("the project is wired the way this file proves to work:")
    problems = static_checks()
    if not problems:
        print("  %s declares its stamp dependency SYMBOLIC and names no live stamp_always"
              % P05.relative_to(ROOT))
    print()

    if args.static_only:
        print("THE GENERATOR HALF DID NOT RUN, because --static-only was given. The static half")
        print("cannot tell whether the wiring works, only that it is the wiring that was proved")
        print("to work elsewhere. scripts/test_build_stamp.py with no argument is the real test.")
    else:
        cmake = shutil.which("cmake")
        ninja = shutil.which("ninja")
        missing = [n for n, v in (("cmake", cmake), ("ninja", ninja)) if v is None]
        if missing:
            print("CANNOT RUN THE GENERATOR HALF: no %s on PATH." % " and no ".join(missing))
            print("That is a refusal and not a pass, because a stamp gate that quietly skips the")
            print("only half that builds anything is the kind of guard this file exists about.")
            print("Use --static-only to ask for the Python half on purpose.")
            return 2
        print("four wirings, each configured once and built twice:")
        problems += dynamic_checks({"cmake": cmake, "ninja": ninja})
        built = True
        print()

    if problems:
        print("REFUSED:")
        for p in problems:
            print("  - %s" % p)
        return 1

    # THE CLOSING LINE SAYS ONLY WHAT RAN. The first version of this file printed the generator
    # half's conclusion after --static-only as well, which is a gate claiming a result it did not
    # take, in a file about a guard that claimed an age the image did not have.
    if built:
        print("the build stamp regenerates at every build, and two wirings that do not are shown"
              " not to")
    else:
        print("the wiring is the one proved to work elsewhere, and nothing was built here to"
              " confirm it")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
