#!/usr/bin/env python3
"""Does check_object_identity.py read the machine from the object, and refuse a mismatched pair?

    python3 scripts/test_check_object_identity.py

WHY THIS EXISTS. That script compares one application object between two builds and is used by
three chapters: P03's two buses, P14's second processor family, and P05's criterion 6. For its
first year it hard-coded the Zephyr SDK's ARM binutils, which was correct for the cross builds it
was written for and made it unusable for the host builds P05 needs. On Thursday 19 November 2026
it was pointed at two `native_sim` objects and the ARM objdump answered "can't disassemble for
architecture UNKNOWN". **It refused rather than passing, which was the right failure**, and it
could not do the comparison at all.

It now reads `e_machine` out of the ELF header and picks binutils accordingly, and it refuses a
pair of objects built for different machines. That refusal is the one worth testing: comparing an
ARM object with an x86 one would differ in every line while saying nothing about the source, and
projects/05-the-zones/docs/CRITERION6.md names it as a way that comparison could appear to run
and prove nothing.

WHAT IS TESTED HERE AND WHAT IS NOT. The header reading and the mismatch refusal are, with
synthetic ELF headers, in both byte orders. The comparison itself is not: a twenty-byte header is
not something objdump can disassemble, so the agreeing case needs real objects and is exercised
by `code.yml`, which runs the script on P03's two builds on every push.
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "scripts" / "check_object_identity.py"
OBJ = "CMakeFiles/app.dir/src/main.c.obj"

ARM, X86_64, I386, UNKNOWN = 0x28, 0x3E, 0x03, 0xF3


def elf_header(machine: int, big: bool = False) -> bytes:
    """The first twenty bytes of an ELF file, which is all the architecture check reads."""
    h = bytearray(20)
    h[0:4] = b"\x7fELF"
    h[4] = 2                      # 64-bit, not read by the check
    h[5] = 2 if big else 1        # the byte order the check honours
    if big:
        h[18], h[19] = machine >> 8, machine & 0xFF
    else:
        h[18], h[19] = machine & 0xFF, machine >> 8
    return bytes(h)


def plant(root: Path, name: str, machine: int, big: bool = False) -> Path:
    """A build directory carrying one object at the path the script expects."""
    p = root / name / OBJ
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(elf_header(machine, big))
    return root / name


def run_script(a: Path, b: Path, cwd: Path):
    r = subprocess.run([sys.executable, str(SCRIPT), str(a), str(b)],
                       cwd=str(cwd), capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def main() -> int:
    if not SCRIPT.exists():
        print("REFUSED: %s is not there, so there is nothing to test." % SCRIPT.name)
        return 2
    sys.path.insert(0, str(ROOT / "scripts"))
    import check_object_identity as checker

    results = []

    # The header reading, both byte orders, and the refusals.
    for machine, want, label in ((ARM, "arm", "an ARM object"),
                                 (X86_64, "host", "an x86-64 object"),
                                 (I386, "host", "an i386 object"),
                                 (UNKNOWN, None, "a machine it has no binutils for")):
        for big in (False, True):
            with tempfile.TemporaryDirectory(prefix="objid") as tmp:
                p = Path(tmp) / "o"
                p.write_bytes(elf_header(machine, big))
                got, why = checker.architecture(p)
                order = "big-endian" if big else "little-endian"
                results.append(("%s, %s, reads as %s" % (label, order, want), got == want))
                if want is None:
                    results.append(("and the reason names the machine",
                                    "0x%02x" % machine in why.lower()))

    with tempfile.TemporaryDirectory(prefix="objid") as tmp:
        root = Path(tmp)

        # Not an ELF file at all.
        p = root / "notelf"
        p.write_bytes(b"this is not an object")
        got, why = checker.architecture(p)
        results.append(("a file that is not ELF is refused", got is None))
        results.append(("and the reason says so", "not an ELF object" in why))

        # A file shorter than the header.
        p = root / "tiny"
        p.write_bytes(b"\x7fELF")
        got, _ = checker.architecture(p)
        results.append(("a file shorter than the header is refused", got is None))

        # THE CASE THIS TEST EXISTS FOR: two builds for different machines.
        arm_build = plant(root, "build-arm", ARM)
        host_build = plant(root, "build-host", X86_64)
        rc, out = run_script(arm_build, host_build, root)
        results.append(("a pair for different machines refuses", rc == 1))
        results.append(("and says there is nothing to compare",
                        "different machines" in out))
        results.append(("and names both objects", "is arm" in out and "is host" in out))

        # Two builds for the same machine get past the architecture check and then stop at
        # objdump, which cannot disassemble twenty bytes. That is the boundary of this test.
        same = plant(root, "build-arm2", ARM)
        rc, out = run_script(arm_build, same, root)
        results.append(("a matching pair gets past the machine check",
                        "both objects are arm" in out))

        # A missing object is refused before anything else.
        rc, out = run_script(arm_build, root / "build-absent", root)
        results.append(("a missing object refuses", rc == 1))
        results.append(("and names the path it looked for", "no object at" in out))

    for label, ok in results:
        print("  %-54s %s" % (label, "yes" if ok else "NO"))
    passed = sum(1 for _, ok in results if ok)
    print()
    print("%d of %d checks pass" % (passed, len(results)))
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
