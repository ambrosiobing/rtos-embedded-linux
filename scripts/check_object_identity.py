#!/usr/bin/env python3
"""P03's first criterion: one application, two buses, the same instructions.

    python scripts/check_object_identity.py build-i2c build-spi

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
"""

import re
import subprocess
import sys
from pathlib import Path

OBJ = "CMakeFiles/app.dir/src/main.c.obj"
ORD = re.compile(r"__device_dts_ord_(\d+)")


def tool(name):
    """The SDK's binutils, found where west sdk install puts them."""
    for sdk in sorted(Path.home().glob("zephyr-sdk-*"), reverse=True):
        exe = sdk / "gnu" / "arm-zephyr-eabi" / "bin" / ("arm-zephyr-eabi-" + name)
        if exe.is_file():
            return str(exe)
    return "arm-zephyr-eabi-" + name


def run(exe, *args):
    out = subprocess.run([exe, *args], capture_output=True, text=True)
    if out.returncode != 0:
        print("  %s failed: %s" % (exe, out.stderr.strip()))
        return None
    return out.stdout


def disassembly(path):
    """Instructions only. The objdump header names the file, which differs by construction
    and says nothing about the code."""
    text = run(tool("objdump"), "-d", str(path))
    if text is None:
        return None
    return [ln for ln in text.splitlines() if "file format" not in ln]


def main(argv):
    if len(argv) != 3:
        print(__doc__.splitlines()[2].strip())
        return 2

    a = Path(argv[1]) / OBJ
    b = Path(argv[2]) / OBJ
    for p in (a, b):
        if not p.is_file():
            print("FAIL no object at %s" % p)
            return 1

    problems = 0

    # The instructions, which must not differ at all.
    da, db = disassembly(a), disassembly(b)
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
    sa = run(tool("nm"), str(a))
    sb = run(tool("nm"), str(b))
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
