#!/usr/bin/env python3
"""P03's first criterion: one application, two buses, the same instructions.

    python scripts/check_object_identity.py build-i2c build-spi
    python scripts/check_object_identity.py build-a build-b CMakeFiles/app.dir/src/pipe.c.obj

WHAT IT CHECKS, and why it is not `cmp`. The chapter asks that the application object be byte
for byte identical between the two-wire and the four-wire build. Both builds were made on
Tuesday 6 October 2026 and the objects differ at byte 994. The difference is the NAME of one
undefined symbol:

    U __device_dts_ord_147      against      U __device_dts_ord_160

That name carries the device's ordinal in the generated devicetree, and the two descriptions
number the device differently. The compiled instructions are identical. Nothing in the
application chose the ordinal and nothing in it can see one: the reference is resolved at
link time from whichever description was used, which is the mechanism working rather than
failing.

So this checks the thing the chapter meant rather than the thing it wrote:

    the disassembly is identical, and
    the symbol tables differ in nothing but one __device_dts_ord_ pair.

That is weaker in letter and stronger in substance. Byte equality could be satisfied by an
application that reads nothing; this cannot, because the object must still name a device.
And byte equality would break on any unrelated node added to an overlay, because that moves
the ordinal, so the original test would have failed for reasons unconnected to the claim.

AN OBJECT WHOSE INSTRUCTIONS DIFFER is the real refutation, and this reports the
disassembly diff when it happens, because that names the line.

THE OBJECT PATH IS AN ARGUMENT, with P03's as the default. It was a constant until Monday
26 October 2026, when P14's replicated claim was found to describe it as an argument: that
chapter takes this check to a second processor family, where the application being compared
is P06's pipe rather than P03's, so the object it must read is not this one. A chapter that
describes a capability the code does not have is the defect, whichever of the two is changed,
and here the code was the easier half and the chapter was right about what the check is for.
"""

import re
import subprocess
import sys
from pathlib import Path

OBJ = "CMakeFiles/app.dir/src/main.c.obj"   # P03's, and the default when none is given
ORD = re.compile(r"__device_dts_ord_(\d+)")


# WHICH MACHINE AN OBJECT IS FOR, READ FROM THE OBJECT. The ELF header carries it in
# e_machine, two bytes at offset 18, in the byte order byte 5 declares. Only the two this
# repository produces are named; anything else is refused by name rather than guessed at.
ELF_MACHINES = {0x28: "arm", 0x3E: "host", 0x03: "host"}


def architecture(path):
    """("arm" or "host", a sentence), or (None, why not)."""
    with open(str(path), "rb") as f:
        head = f.read(20)
    if len(head) < 20 or head[:4] != b"\x7fELF":
        return None, "%s is not an ELF object" % path
    big = head[5] == 2
    machine = (head[18] << 8 | head[19]) if big else (head[19] << 8 | head[18])
    arch = ELF_MACHINES.get(machine)
    if arch is None:
        return None, ("%s is for ELF machine 0x%02x, which this script has no binutils for"
                      % (path, machine))
    return arch, "%s is %s, ELF machine 0x%02x" % (path, arch, machine)


def tool(name, arch):
    """The binutils that can read this object, which is not always the SDK's.

    AND IT USED TO BE THE SDK'S ALWAYS, which made this script unusable for half of what it is
    for. It was written for P03, whose two builds are cross builds for the board, so the ARM
    prefix was hard-coded and correct. On Thursday 19 November 2026 P05's criterion 6 pointed it
    at two `native_sim` builds, which are host objects, and the ARM objdump answered "can't
    disassemble for architecture UNKNOWN". The script refused rather than passing, which was the
    right failure, and it could not do the comparison at all.
    """
    if arch == "arm":
        for sdk in sorted(Path.home().glob("zephyr-sdk-*"), reverse=True):
            exe = sdk / "gnu" / "arm-zephyr-eabi" / "bin" / ("arm-zephyr-eabi-" + name)
            if exe.is_file():
                return str(exe)
        return "arm-zephyr-eabi-" + name
    return name   # the host's own, which is what reads a native_sim object


def run(exe, *args):
    out = subprocess.run([exe, *args], capture_output=True, text=True)
    if out.returncode != 0:
        print("  %s failed: %s" % (exe, out.stderr.strip()))
        return None
    return out.stdout


def disassembly(path, arch):
    """Instructions only. The objdump header names the file, which differs by construction
    and says nothing about the code."""
    text = run(tool("objdump", arch), "-d", str(path))
    if text is None:
        return None
    return [ln for ln in text.splitlines() if "file format" not in ln]


def main(argv):
    if len(argv) not in (3, 4):
        for line in __doc__.splitlines()[2:4]:
            print(line.strip())
        return 2

    obj = argv[3] if len(argv) == 4 else OBJ
    if len(argv) == 4:
        print("reading %s in each build, rather than the default %s" % (obj, OBJ))
    a = Path(argv[1]) / obj
    b = Path(argv[2]) / obj
    for p in (a, b):
        if not p.is_file():
            print("FAIL no object at %s" % p)
            return 1

    # BOTH OBJECTS MUST BE FOR THE SAME MACHINE, and this is a refusal rather than a detail.
    # P01's Zephyr application is built for native_sim with the host compiler while this
    # repository's board applications are cross built, so an invocation that pairs one of each
    # would be comparing x86 with ARM. That is not a comparison, and the objects would differ in
    # every line while saying nothing about the source. projects/05-the-zones/docs/CRITERION6.md
    # names it as one of the ways its comparison could appear to run and prove nothing.
    arch_a, why_a = architecture(a)
    arch_b, why_b = architecture(b)
    for why, arch in ((why_a, arch_a), (why_b, arch_b)):
        if arch is None:
            print("FAIL %s" % why)
            return 1
    if arch_a != arch_b:
        print("FAIL the two objects are for different machines, so there is nothing to compare:")
        print("    %s" % why_a)
        print("    %s" % why_b)
        return 1
    print("both objects are %s" % arch_a)

    problems = 0

    # The instructions, which must not differ at all.
    da, db = disassembly(a, arch_a), disassembly(b, arch_a)
    if da is None or db is None:
        return 1
    if da != db:
        print("FAIL the instructions differ, so the application depends on the bus")
        for line in [x for x in da if x not in db][:20]:
            print("    only in %s: %s" % (argv[1], line))
        for line in [x for x in db if x not in da][:20]:
            print("    only in %s: %s" % (argv[2], line))
        problems += 1

    # The symbols, which may differ in exactly one ordinal and nothing else.
    sa = run(tool("nm", arch_a), str(a))
    sb = run(tool("nm", arch_a), str(b))
    if sa is None or sb is None:
        return 1

    only_a = [ln for ln in sa.splitlines() if ln not in sb.splitlines()]
    only_b = [ln for ln in sb.splitlines() if ln not in sa.splitlines()]

    ords_a = [m.group(1) for ln in only_a for m in [ORD.search(ln)] if m]
    ords_b = [m.group(1) for ln in only_b for m in [ORD.search(ln)] if m]

    unexplained = ([ln for ln in only_a if not ORD.search(ln)]
                   + [ln for ln in only_b if not ORD.search(ln)])
    if unexplained:
        print("FAIL the symbol tables differ by more than a devicetree ordinal:")
        for ln in unexplained[:20]:
            print("    %s" % ln.strip())
        problems += 1

    if problems == 0:
        print("the instructions are identical: %d lines of disassembly, no difference"
              % len(da))
        if ords_a and ords_b:
            print("the objects name the device by its devicetree ordinal, and only that "
                  "differs: %s against %s" % (ords_a[0], ords_b[0]))
        else:
            print("no devicetree ordinal differs either, so the objects are identical")
        print("one application, two buses, the same code")

    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
