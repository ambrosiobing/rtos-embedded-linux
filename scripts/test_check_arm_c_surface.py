#!/usr/bin/env python3
"""Does check_arm_c_surface.py pass a tree that satisfies it and refuse one that does not?

    python3 scripts/test_check_arm_c_surface.py

That script decides whether arm C of projects/04-what-the-kernel-primitives-cost/docs/SETTLING.md
is worth a second Zephyr workspace and the development kit of its era, so its verdict is acted
on rather than read. A checker whose refusal has never been seen is not a checker, which this
repository has now learned from a pin that lived in a comment, a gate that matched its own prose,
and a build stamp written once per build directory.

THREE SYNTHETIC CLONES, tagged and read the same way the real one will be. No Zephyr, no
toolchain and no network: git and this interpreter are the whole requirement, which is why this
runs in the Python-only workflow.

    a tree matching every prediction        must pass, and must say the predictions held
    the same tree missing one bracket name  must refuse with exit 1, naming that name
    a tag that is not in the clone          must refuse with exit 2 rather than report anything

The third case is the one worth having. A checker that answers questions about a tag it cannot
see would report every name as absent and read as a catastrophic refusal, which is the same
defect as a gate that passes when it cannot run.
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "scripts" / "check_arm_c_surface.py"

HEADERS = [
    "include/zephyr/kernel.h",
    "include/zephyr/sys/util.h",
    "include/zephyr/drivers/gpio.h",
    "include/zephyr/timing/timing.h",
]

SYMBOLS = [
    "timing_init", "timing_start", "timing_counter_get", "timing_freq_get",
    "k_cycle_get_32", "sys_clock_hw_cycles_per_sec",
    "k_sem_take", "k_sem_give", "k_mutex_lock", "k_mutex_unlock",
    "k_yield", "k_thread_suspend", "k_thread_resume", "k_current_get",
    "k_poll", "k_poll_event_init", "k_work_submit",
    "k_thread_create", "k_thread_priority_set", "k_msleep", "k_uptime_get",
    "DT_PATH", "DT_GPIO_CTLR", "DT_NODE_FULL_NAME",
]

# cmsis_core.h, ICACHE and DCACHE are left out on purpose: the script predicts them absent, so a
# fixture that supplied them would refute the prediction rather than test the checker.
KCONFIG = [
    "POLL", "ASSERT", "TIMESLICING", "TIMESLICE_SIZE", "TIMING_FUNCTIONS",
    "HW_STACK_PROTECTION", "ARM_MPU", "FPU_SHARING", "PM", "BOARD_NUCLEO_H7A3ZI_Q",
]


def run(args, cwd, check=True):
    r = subprocess.run([str(a) for a in args], cwd=str(cwd),
                       capture_output=True, text=True)
    if check and r.returncode != 0:
        raise RuntimeError("setup failed: %s\n%s\n%s"
                           % (" ".join(str(a) for a in args), r.stdout[-600:], r.stderr[-600:]))
    return r


def build(root: Path, omit=()) -> Path:
    root.mkdir(parents=True)
    for h in HEADERS:
        p = root / h
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text("/* %s */\n" % h, encoding="utf-8")

    decls = ["/* synthetic declarations, enough for a presence check */"]
    for s in SYMBOLS:
        if s in omit:
            continue
        decls.append("#define %s(node) node" % s if s.startswith("DT_")
                     else "int %s(void);" % s)
    (root / "include" / "zephyr" / "kernel.h").write_text(
        "\n".join(decls) + "\n", encoding="utf-8")

    (root / "Kconfig").write_text(
        "\n".join("config %s\n\tbool\n" % k for k in KCONFIG), encoding="utf-8")

    run(["git", "init", "-q"], root)
    run(["git", "add", "-A"], root)
    run(["git", "-c", "user.email=t@t", "-c", "user.name=t",
         "commit", "-q", "-m", "synthetic tree"], root)
    run(["git", "tag", "v3.0.0"], root)
    return root


def main() -> int:
    if not SCRIPT.exists():
        print("REFUSED: %s is not there, so there is nothing to test."
              % SCRIPT.relative_to(ROOT))
        return 2

    checks = []
    with tempfile.TemporaryDirectory(prefix="armcgate") as tmp:
        work = Path(tmp)

        good = build(work / "good")
        r = run([sys.executable, SCRIPT, good, "v3.0.0"], work, check=False)
        checks.append(("a satisfying tree passes", r.returncode == 0))
        checks.append(("and reports that the predictions held",
                       "Every prediction held" in r.stdout))
        checks.append(("and finds no bracket name missing",
                       "No name used inside a bracket is missing" in r.stdout))
        # COUNTED AS ROWS AND NOT AS WORDS. The first version of this check counted the word
        # ABSENT and expected three, and the summary line "from the ABSENT rows above" made it
        # four, so a correct script failed its own test. A count is only a count of the thing it
        # names.
        absent_rows = [line for line in r.stdout.splitlines()
                       if " ABSENT " in line and ("header" in line or "kconfig" in line)]
        checks.append(("and still reports the three predicted absences",
                       len(absent_rows) == 3))

        bad = build(work / "bad", omit=("k_poll",))
        r2 = run([sys.executable, SCRIPT, bad, "v3.0.0"], work, check=False)
        checks.append(("a missing bracket name refuses", r2.returncode == 1))
        checks.append(("and the refusal names it", "- k_poll" in r2.stdout))
        checks.append(("and says not to build the workspace",
                       "do not build the second workspace" in r2.stdout))
        # k_poll_event_init is still declared, so a substring match would have found k_poll and
        # reported a pass. The word boundary is what makes the refusal real.
        checks.append(("and k_poll_event_init does not satisfy k_poll",
                       "k_poll_event_init" not in r2.stdout.split("REFUSED AT THIS TAG")[-1]))

        r3 = run([sys.executable, SCRIPT, good, "v2.6.0"], work, check=False)
        checks.append(("an absent tag refuses with its own code", r3.returncode == 2))
        checks.append(("and says it is a refusal and not a result",
                       "not a result" in r3.stdout))

        r4 = run([sys.executable, SCRIPT, work / "nothing-here", "v3.0.0"], work, check=False)
        checks.append(("a path that is not a clone refuses", r4.returncode == 2))

        r5 = run([sys.executable, SCRIPT], work, check=False)
        checks.append(("no arguments prints the usage and refuses", r5.returncode == 2))

    for label, ok in checks:
        print("  %-52s %s" % (label, "yes" if ok else "NO"))
    passed = sum(1 for _, ok in checks if ok)
    print()
    print("%d of %d checks pass" % (passed, len(checks)))
    return 0 if passed == len(checks) else 1


if __name__ == "__main__":
    raise SystemExit(main())
