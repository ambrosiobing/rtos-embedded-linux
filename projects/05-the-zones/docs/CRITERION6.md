# Criterion 6: P01 is unchanged, and the only honest way to show it is byte for byte

Written at a desk on Thursday 19 November 2026, with no board and nothing measured. This settles
what the remaining half of criterion 6 is, which object it compares and what would refute it,
before the build that produces it exists. `docs/DESIGN.md` sorts the chapter's seven criteria by
what each costs and puts this one in the column that needs **a host only**, ahead of the shield
and ahead of the bench. It has been the next host-only item since Monday 26 October 2026, when
the first of its two blockers was cleared.

## What the criterion claims, in the narrow form

The chapter's loose reading is that this part is interchangeable with any other range source. The
narrow form, which `docs/DESIGN.md` adopted because it names the thing that had to be added, is
two statements:

1. **P01's application object is byte for byte what it was.**
2. **The file that chooses one number from sixty-four is new, and it is not P01's.**

The second is already true and visible: `src/reduce_nearest_valid.c` is this project's, has its
own host suite under `host/`, and `docs/REDUCTION.md` says what it reduces. The first is the half
that has never been shown.

## Why `presence.c` is the right object, and this is the fact that makes the check strong

`projects/01-presence/c/presence.c` includes exactly two things, `presence.h` and `<string.h>`,
and `presence.h` includes only `<stdbool.h>`, `<stddef.h>` and `<stdint.h>`. **It contains no
conditional compilation and names no `CONFIG_` symbol at all**, measured on Thursday 19 November
2026.

That matters more than it sounds. It means the object this criterion compares cannot depend on
the operating system, on the application built around it, on a Kconfig choice, or on which
project's build produced it. Everything that could make the two objects differ is therefore
either a real change to P01 or a difference in how the compiler was invoked, and those two are
distinguishable. **A criterion that compares an object whose content depends on its surroundings
proves nothing**, and this one does not have that problem.

## The two builds, and the one that does not exist yet

**Both builds must target the same board, and that is a requirement rather than a
convenience.** P01's Zephyr application is built for `native_sim`, which compiles with the host
compiler; this project's own builds target the Nucleo through the cross toolchain. An object from
one and an object from the other differ in their instruction set, so comparing them would be
comparing x86 with ARM and would mean nothing at all. The comparison therefore happens where the
reference already is.

| | build | board | what posts the reading |
|---|---|---|---|
| the reference | P01's own Zephyr application, unchanged | `native_sim` | P01's own source of readings |
| the subject | an application linking P01's `presence.c`, `presence_adapter.c` and `phases.c` unchanged, plus this project's `src/reduce_nearest_valid.c` and `src/zone_status.h` | `native_sim` | the reduction, over a grid supplied by the test rather than by the part |

**The subject build needs no board, no shield, no vendor clone and no cross toolchain**, and
that is worth stating because this page said the opposite when it was first written on Thursday
19 November 2026. The reason it needs none of them is that `src/reduce_nearest_valid.c` includes
only its own header and `src/zone_status.h`: it names no operating system and no vendor
interface, which is also why it already has a host suite under `host/`. So the claim this
criterion makes can be shown with a grid the test writes.

**That also means it belongs in continuous integration rather than in a sitting at the build
machine.** `code.yml` already configures and runs the reference, `cmake -B build-zephyr -GNinja
-DBOARD=native_sim -S projects/01-presence/zephyr`, and gates it on the printed
`PASSED: 0 failure(s)`. A second configure beside it and one call to the object checker is the
whole of the remaining work, and then the criterion is enforced on every push instead of being
true on the day somebody looked.

The integration is still the real content: an application that wires the reduction's one number
into `presence_adapter_post` and does nothing else.

## What compares them, and why the checker already takes an argument

`scripts/check_object_identity.py` does this comparison. Its object path became an argument on
Monday 26 October 2026 because P14's replicated claim described it that way, and **criterion 6
needs that argument for its own reason**: the object to compare here is P01's machine and not an
application's `main.c`, so the default is the wrong file.

    python scripts/check_object_identity.py <reference build> <subject build> \
        CMakeFiles/app.dir/<path to>/presence.c.obj

The script reports a disassembly difference when the instructions differ, which is what names the
line rather than only the fact.

## What refutes this, written before the run

**Refuted if the two objects differ in their instructions.** That is a real change to P01 and the
claim of interchangeability fails, whatever the chapter says.

**And refuted in a second way that is easier to miss: if the two compile commands differ in
anything that reaches code generation.** Two different Zephyr applications have different
`prj.conf` files, so they have different generated `autoconf.h`, and Zephyr force-includes it.
`presence.c` names no `CONFIG_` symbol, so that difference cannot reach its object, but the
optimisation level, the architecture flags and the standard version can. **A difference there
makes this criterion either vacuous or spuriously red, and neither is a result.** So the run
records both compile command lines for that one object and shows them equal except for the output
path.

**Refuted a third way if the two builds did not compile the same source.** The objects can only
be compared if both read the same `presence.c` and the same `presence.h`, so the run records the
hash of each and shows them equal. An include path that reaches a copy would otherwise satisfy
the check while proving the opposite of what it claims.

## What this does not prove, stated so the next step is not read as smaller

It does not prove the integration runs. A byte-identical object says P01's machine was not
touched; it says nothing about whether the reduction feeds it a sensible number, which needs the
part, the shield and a scene. Those belong to criteria 1 to 5 and to the bench.

It also does not prove the reduction is right. `host/` does that, on a host, by building the
reduction and breaking it four ways with `make mutants`.

## Order, and what it costs

**This section is a correction.** It first read that the subject build needs the cross toolchain
and the vendor clone, written before P01's build file had been read. It needs neither, for the
reasons above, and it must not use them: the reference is a `native_sim` build and an object
compared across two architectures is not a comparison. The error was assuming this project's
build shape applied to P01's, which is the opposite of predicting from contents.

So the cost is a second `native_sim` configure and one call to the object checker, both of which
run wherever the Zephyr tree is, including the workflow that already builds the reference. That
is cheaper than `docs/DESIGN.md` promised when it put this criterion in the host-only column,
and it is the reason that column was right.

## Sources

- `docs/DESIGN.md` of this project, for the table that sorts the seven criteria by what each
  needs and for the narrow form of this one.
- `projects/01-presence/c/presence.c` and `presence.h`, read on Thursday 19 November 2026 for the
  two includes, the absence of conditional compilation and the absence of any `CONFIG_` symbol.
- `projects/01-presence/adapter/presence_adapter.h` for the one function the subject build has to
  call, and `projects/01-presence/zephyr/presence_adapter.c` for how a reading is stamped.
- `scripts/check_object_identity.py` for the comparison and for the object-path argument this
  criterion needs.
- `docs/REDUCTION.md` and `host/` for the reduction and the suite that already proves it, and
  `src/reduce_nearest_valid.c` itself, read on Thursday 19 November 2026 for the two includes
  that make it free of the operating system and of the vendor.
- `projects/01-presence/zephyr/CMakeLists.txt` for the board the reference is built for, and
  `.github/workflows/code.yml` for the configure line and the gate that already run it.
