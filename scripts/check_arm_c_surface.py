#!/usr/bin/env python3
"""Is this project's Zephyr harness buildable against an older Zephyr without becoming a
different harness? Asked of a tag in a clone, with no configure and no toolchain.

    python3 scripts/check_arm_c_surface.py <path to a zephyr clone> [tag, default v3.0.0]

WHY THIS EXISTS. Arm C of projects/04-what-the-kernel-primitives-cost/docs/SETTLING.md measures
the same bracket under the oldest Zephyr this board exists in, which that page establishes is
3.0.0. It became first in the order on Monday 26 October 2026, when arms A and B excluded the
FreeRTOS side and left the Zephyr version as the one candidate for the published factor.

**The obstacle is not the second workspace.** sections/z04.tex refused the originally proposed
run partly because it moved the harness and the kernel version together. Arm C inherits that
objection unless the surface is checked first, so SETTLING.md adopts a rule and this script
applies it:

    an include path or a Kconfig spelling may change    they are outside every bracket
    a name used inside a bracket may not                that changes what is measured

AND THE HAZARD IS A FALSE LINE RATHER THAN A FAILED BUILD, which is why this checks names that
would compile fine. `main.c` prints its own provenance with IS_ENABLED(CONFIG_ICACHE), and
IS_ENABLED on a Kconfig symbol that does not exist is 0 with no diagnostic. A symbol renamed
between releases therefore prints a cache as disabled under a hardware line reporting it enabled,
and a reader comparing two captures sees a difference that is only a spelling.

THE PREDICTIONS ARE IN THIS FILE and were written before the tree was read, which is how every
arm on that page is handled. A run that cannot refute them is not a check.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

# (name, kind, in_bracket, predicted_present)
#
# in_bracket marks a name whose absence or change refutes arm C outright rather than costing a
# line of preparation. That column is the whole point of the table: the two are not the same
# finding and must not arrive as one number.
SURFACE = [
    ("include/zephyr/kernel.h", "header", False, True),
    ("include/zephyr/sys/util.h", "header", False, True),
    ("include/zephyr/drivers/gpio.h", "header", False, True),
    ("include/zephyr/timing/timing.h", "header", False, True),
    ("include/cmsis_core.h", "header", False, False),

    ("timing_init", "symbol", False, True),
    ("timing_start", "symbol", False, True),
    ("timing_counter_get", "symbol", False, True),
    ("timing_freq_get", "symbol", False, True),
    ("k_cycle_get_32", "symbol", False, True),
    ("sys_clock_hw_cycles_per_sec", "symbol", False, True),

    ("k_sem_take", "symbol", True, True),
    ("k_sem_give", "symbol", True, True),
    ("k_mutex_lock", "symbol", True, True),
    ("k_mutex_unlock", "symbol", True, True),
    ("k_yield", "symbol", True, True),
    ("k_thread_suspend", "symbol", True, True),
    ("k_thread_resume", "symbol", True, True),
    ("k_current_get", "symbol", True, True),
    ("k_poll", "symbol", True, True),
    ("k_poll_event_init", "symbol", True, True),
    ("k_work_submit", "symbol", True, True),

    ("k_thread_create", "symbol", False, True),
    ("k_thread_priority_set", "symbol", False, True),
    ("k_msleep", "symbol", False, True),
    ("k_uptime_get", "symbol", False, True),

    ("DT_PATH", "symbol", False, True),
    ("DT_GPIO_CTLR", "symbol", False, True),
    ("DT_NODE_FULL_NAME", "symbol", False, True),

    ("POLL", "kconfig", False, True),
    ("ASSERT", "kconfig", False, True),
    ("TIMESLICING", "kconfig", False, True),
    ("TIMESLICE_SIZE", "kconfig", False, True),
    ("TIMING_FUNCTIONS", "kconfig", False, True),
    ("HW_STACK_PROTECTION", "kconfig", False, True),
    ("ARM_MPU", "kconfig", False, True),
    ("FPU_SHARING", "kconfig", False, True),
    ("PM", "kconfig", False, True),
    ("ICACHE", "kconfig", False, False),
    ("DCACHE", "kconfig", False, False),
    ("BOARD_NUCLEO_H7A3ZI_Q", "kconfig", False, True),
]


def git(tree: Path, *args: str) -> tuple[int, str]:
    r = subprocess.run(("git", "-C", str(tree)) + args,
                       capture_output=True, text=True)
    return r.returncode, r.stdout


def header_present(tree: Path, tag: str, path: str) -> tuple[bool, str]:
    """Exact path first, then the basename anywhere, because a header can move rather than go.

    SEARCHING BY CONTENT AND NOT BY LOCATION is this repository's rule for a claim of absence.
    Reporting cmsis_core.h as missing when it has merely moved would send a reader to write a
    porting layer for a file that is on disk.
    """
    rc, _ = git(tree, "cat-file", "-e", "%s:%s" % (tag, path))
    if rc == 0:
        return True, path
    base = path.rsplit("/", 1)[-1]
    rc, out = git(tree, "ls-tree", "-r", "--name-only", tag)
    if rc != 0:
        return False, "the tree could not be listed"
    hits = [line for line in out.splitlines() if line.rsplit("/", 1)[-1] == base]
    if hits:
        return True, "moved, found at " + ", ".join(hits[:3])
    return False, "not at that path and no file of that name in the tree"


def symbol_present(tree: Path, tag: str, name: str) -> tuple[bool, str]:
    rc, out = git(tree, "grep", "-l", "-w", name, tag, "--", "include")
    if rc == 0 and out.strip():
        files = [line.split(":", 1)[-1] for line in out.splitlines()]
        return True, "declared in " + ", ".join(sorted(files)[:2])
    return False, "no declaration under include/"


def kconfig_present(tree: Path, tag: str, name: str) -> tuple[bool, str]:
    for pattern in ("^config %s$" % name, "^menuconfig %s$" % name):
        rc, out = git(tree, "grep", "-l", "-E", pattern, tag)
        if rc == 0 and out.strip():
            files = [line.split(":", 1)[-1] for line in out.splitlines()]
            return True, "defined in " + ", ".join(sorted(files)[:2])
    return False, "no 'config %s' anywhere in the tree" % name


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print(__doc__.splitlines()[2].strip())
        return 2
    tree = Path(argv[1])
    tag = argv[2] if len(argv) == 3 else "v3.0.0"

    if not (tree / ".git").exists() and not tree.joinpath("HEAD").exists():
        print("REFUSED: %s is not a git clone." % tree)
        return 2

    rc, _ = git(tree, "rev-parse", "--verify", tag + "^{commit}")
    if rc != 0:
        print("REFUSED: %s has no tag %s." % (tree, tag))
        print("That is a refusal and not a result. Fetch the tags and run this again:")
        print("    git -C %s fetch --tags origin" % tree)
        return 2

    rc, head = git(tree, "describe", "--tags", "--always")
    print("clone %s, currently %s, reading tag %s" % (tree, head.strip() or "unknown", tag))
    print()

    lookup = {"header": header_present, "symbol": symbol_present, "kconfig": kconfig_present}
    # MOVED IS COUNTED SEPARATELY FROM ABSENT, and that distinction was missing on the first
    # real run. Four of the five headers were reported present with a note saying they had moved,
    # because at v3.0.0 the public headers are not under include/zephyr/ at all, and the summary
    # then said arm C needed "an include path" to respell when it needed five. **A gate that
    # knows something and does not say it in its conclusion is a gate whose conclusion is read
    # instead of its table.**
    surprises, blockers, moved, absent = [], [], [], []

    for name, kind, in_bracket, predicted in SURFACE:
        present, note = lookup[kind](tree, tag, name)
        agreed = present == predicted
        mark = "  " if agreed else "<-"
        print("%s %-34s %-8s %-8s %s" % (
            mark, name, kind, "present" if present else "ABSENT", note[:60]))
        if not agreed:
            surprises.append((name, kind, predicted, present))
        if in_bracket and not present:
            blockers.append(name)
        if present and note.startswith("moved"):
            moved.append((name, note))
        if not present:
            absent.append(name)

    print()
    print("=" * 78)

    if blockers:
        print("ARM C IS REFUSED AT THIS TAG. These names are used inside a bracket and are not")
        print("available, so measuring here would move the harness and the kernel version")
        print("together, which is the ground the originally proposed run was refused on:")
        for b in blockers:
            print("  - %s" % b)
        print()
        print("Record this on the settling page and do not build the second workspace.")
        return 1

    print("No name used inside a bracket is missing, so arm C does not move the measurement.")

    if surprises:
        print()
        print("THE PREDICTIONS WERE WRONG IN %d PLACE(S), which is a result and not a failure."
              % len(surprises))
        for name, kind, predicted, present in surprises:
            print("  - %s (%s): predicted %s, found %s" % (
                name, kind, "present" if predicted else "absent",
                "present" if present else "absent"))
        print()
        print("Update the table in docs/SETTLING.md with what was found, keeping what was")
        print("predicted beside it, and say which way each one moved.")
    else:
        print("Every prediction held, in all %d rows." % len(SURFACE))

    print()
    print("PREPARATION ARM C NEEDS, counting the moved rows and not only the absent ones:")
    print("  %d include path(s) to respell, %d of them because the header moved and %d because"
          % (len(moved) + len([a for a in absent if a.startswith("include/")]),
             len(moved), len([a for a in absent if a.startswith("include/")])))
    print("  it is not in this release at all")
    for name, note in moved:
        print("      %-32s %s" % (name, note))
    for name in absent:
        if name.startswith("include/"):
            print("      %-32s absent, needs the arch-specific spelling of this release" % name)
    kconf = [a for a in absent if not a.startswith("include/")]
    if kconf:
        print("  %d Kconfig symbol(s) to stop asking IS_ENABLED about, since a symbol that does"
              % len(kconf))
        print("  not exist answers 0 with no diagnostic: %s" % ", ".join(kconf))
    print()
    print("Every one of those sits outside every bracket, so none changes a measured quantity.")
    print()
    print("WHAT THIS DOES NOT PROVE. A symbol row says the name occurs in a header at this tag,")
    print("not that its signature is unchanged, and some of the files named above mention a name")
    print("incidentally rather than declaring it. Signatures are settled by the compiler, which")
    print("is the second step and needs the workspace this check was written to justify.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
