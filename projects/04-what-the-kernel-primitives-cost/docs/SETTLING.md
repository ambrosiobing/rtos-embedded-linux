# The settling runs: what 1.10 against 2.35 decomposes into

Written at a desk on Monday 26 October 2026, with no board and no instrument. The eighth
acceptance criterion of this chapter reports that the direction of the gap between the two
kernels is reproduced on three operations and that the size of the context switch gap is not:
1.10 on this board against 2.35 published. It names three candidate reasons and asserts none.

This page does three things. It records that the run the chapter proposed cannot be built. It
reads this project's own measured documents against each other and finds that part of the
question is already answered and that the comparison is not matched. And it replaces the one
proposed run with five arms, in order of what each costs and what each can say, with the
prediction and the refutation for each written before any of them is run.

## The proposed run cannot be built, and the reason is not a judgement

The roadmap sentence of Tuesday 20 October 2026 said: the laboratory's own suite on this board
at the kernel version the laboratory used. Both halves were checked on Monday 26 October 2026.

| Checked | Result |
|---|---|
| `boards/arm/nucleo_h7a3zi_q/nucleo_h7a3zi_q.dts` at Zephyr v2.6.0 | absent, the server returns 404 |
| the same path at v2.7.0 | absent |
| the same path at v3.0.0 | present, the board compatible is `st,stm32h7a3zi-q-nucleo` |
| `dts/arm/st/h7/stm32h7a3.dtsi` at v2.6.0 | absent |

So it is not only the board description that is missing from the laboratory's kernel version but
the part itself. **The oldest Zephyr that can be measured on this board is 3.0.0.** The published
figures were taken at 2.6.0, and no run on this bench can reach them.

The suite is a second matter and is in better shape. The laboratory's Zephyr port of RTOSBench is
public, at `methodpark/zephyr-rtosbench`, BSD-3-Clause, a Zephyr application that builds for any
board the tree supports. Its FreeRTOS half is not public: the original, `gchamp20/RTOSBench`,
keeps its FreeRTOS work on a branch that is not master because of the toolchain it needs, and the
laboratory's own account says porting to FreeRTOS is harder than porting to Zephyr. Running the
suite under FreeRTOS on this board therefore means writing the porting layer the suite documents,
a configuration header and one implementation file.

The proposed run also moved two things at once, the harness and the kernel version, which is the
shape this volume refuses elsewhere. The five arms below each move one.

## What this project has already measured, and what follows from it

Every figure here is from this project's own documents and every one was measured. Counts are of
the cycle counter at 280 MHz, warm, the minimum of sixty-four.

| Figure | Counts per context switch | Where it comes from | Per-switch safety configuration |
|---|---|---|---|
| Zephyr, this board | 345.5 | RESULTS.md, the yield row halved, Friday 9 October 2026 at 09:40 | the board defconfig: an MPU stack guard, reprogrammed at every switch |
| Zephyr, this board | 185 | CRITERION2.md, Friday 9 October 2026 at 21:15 | the guard off, as the kernel's own benchmark sets it |
| Zephyr's own in-tree benchmark, this board | 188 | CRITERION2.md, the same evening | the guard off |
| FreeRTOS, this board | 313 | RESULTS.md, the yield row halved | `configCHECK_FOR_STACK_OVERFLOW` at 2, a pattern check at every switch |
| Zephyr, the laboratory's board | 524 | published, a Cortex-M4 at 168 MHz, Zephyr 2.6.0 | not published |
| FreeRTOS, the laboratory's board | 223 | published, the same board, FreeRTOS 10.4.4 | not published |

Two things follow, and neither is in the chapter as it stands.

**On the Zephyr side the harness is already excluded as a cause.** This project's own bracket,
halved as the second criterion specified before any comparison, and the kernel's own benchmark
suite agree to 0.98 at matched configuration: 185 against 188. Whatever the gap is, it is not
this project measuring a different span from a standard harness, at least under Zephyr. Nothing
of the kind has been done under FreeRTOS, whose 313 has never been compared with any harness but
this one.

**The 1.10 is not a matched comparison, and the chapter does not say so.** It puts a Zephyr that
reprograms a memory protection region at every switch against a FreeRTOS that compares a stack
pattern at every switch. The first costs 178 counts and was measured, by two runs differing in
one setting, on Friday 9 October 2026. The second has never been measured at all. With Zephyr's
guard off and FreeRTOS unchanged the ratio is 185 against 313, which is **0.59, and the direction
reverses**: Zephyr becomes the cheaper of the two. The eighth criterion says "at each kernel's
default configuration", so it is not refuted by this, and the direction it reproduces holds in
one configuration pairing and reverses in another. That belongs in the chapter and is put there
in the same commit as this page.

## The five arms, in the order they are worth running

| Arm | What it settles | What it costs | Can this bench run it |
|---|---|---|---|
| A. The FreeRTOS per-switch check, on and off | the one unmeasured term in the comparison, which is what makes a matched ratio possible for the first time | one setting changed, two runs, the board; no new code and no new tooling | **run on Monday 26 October 2026** |
| B. FreeRTOS 10.4.4 against 11.1.0 | the FreeRTOS half of the version candidate, in full, because this project carries its own board port and does not depend on the kernel supporting the board | one checkout, the pin stated on the command line, and one version guard | **run on Monday 26 October 2026** |
| C. Zephyr 3.0.0 against the pinned 4.5.0-rc1 | the Zephyr half of the version candidate, partly: it cannot reach 2.6.0, where neither the board nor the part exists | a second workspace and the development kit of that era | partly |
| D. The laboratory's own suite on this board | whether its span is the span the kernel's own benchmark measures, and the first independent check of the FreeRTOS figure | a build for the Zephyr half; a porting layer, a configuration header and one file, for the FreeRTOS half | yes for one half, after work for the other |
| E. The laboratory's board, a Cortex-M4 at 168 MHz | the core candidate | a board the bench does not have | no |

Arms A and B need one bench session between them and no new tooling, which is why they are first.
Arm E is named so that the chapter cannot later imply the decomposition was complete.

## What arms A and B measured, on Monday 26 October 2026

Both predictions below were written before either ran. The full record of the four builds and three captures is in [BENCH.md](BENCH.md).

**Arm A is refuted, and the bound is worth more than the value would have been.** Removing the check did not lower the yield row, it raised it by 4 counts to 630. All six rows moved by small amounts in both directions, none beyond the build bound, and the image shrank by 112 bytes so every address after the removal moved. The yield row has now read 626, 626, 626 and 630 across four builds, a spread of 4 counts, so a per-switch cost would have pulled it down by twice itself and did not. **The check costs a few counts per switch at most, against the guard's measured 178 on the other kernel.** The two-run method has a floor and this quantity is under it; the arm's own refutation clause called that outcome a path not reached, and the better reading is a quantity too small for the method.

**Arm B's prediction is met: the FreeRTOS kernel version is not a term.** On the four rows that are not layout-dominated the two versions agree to within a few tenths of a per cent, the yield row reading 622 at V10.4.4 against 626 at V11.1.0. Before it could be measured the build stopped on the one portability difference between the two versions that this project's code meets: the two static-allocation hooks are declared with `uint32_t` in V10.4.4 and with `configSTACK_DEPTH_TYPE` in V11.1.0, so an application providing them needs a version guard. The guard is committed, and its control is that the pinned image is 27148 bytes again, byte for byte what it was before the guard existed.

**What the two leave is one candidate rather than four.** Per switch, this board gives 311 counts at the laboratory's own kernel version where the laboratory measured 223. Both kernels were measured here on one board with one bracket, so neither the board nor the bracket can explain a difference between them:

| Kernel | published | this board | ratio to published |
|---|---|---|---|
| FreeRTOS, the laboratory's own version | 223 | 311 | 1.39 |
| Zephyr, three versions newer, guard off | 524 | 185 | 0.35 |

The published 2.35 did not dissolve on the FreeRTOS side. It dissolved on the Zephyr side, and the candidate left there is the kernel version, which is arm C. That makes arm C the one worth the second workspace it costs, where before today it was third in the order.

**One defect came out of the session and it is in the build rather than in a figure.** The image did not depend on which revision of the two clones it was built from, so moving the kernel clone back and rebuilding recompiled nothing and left the other version's image in place with the pin check passing. No figure of the day is affected, because each build printed its own size and each banner its own stamp. The fix, a stamp file holding both resolved revisions with the image depending on it, is written and has not been run.

## The predictions as they were written, and what refutes each

**Arm A, the FreeRTOS per-switch check.** The setting is `configCHECK_FOR_STACK_OVERFLOW`, which
the board build carries at 2 and the host build at 0. Method 2 compares a short fixed pattern at
the end of a stack on every switch, where Zephyr's guard reprograms a protection region. A
comparison of a handful of words is tens of counts on a core that issues one or two instructions
a cycle, not hundreds. **Predicted: 10 to 80 counts per switch**, by the same two-run method that
measured the guard. *Refuted if* it exceeds the guard's 178, which would mean the software check
costs more than the hardware one and the chapter reports that; or if it is indistinguishable from
zero over sixty-four runs, which would mean the path is not reached and the configuration line the
run prints is describing something that does not happen.

**Arm B, FreeRTOS 10.4.4 against 11.1.0.** Same bracket, same board, same configuration, the one
clone moved by a checkout. The published figure is 223 cycles on a Cortex-M4 and this project's
is 313 on a Cortex-M7, and an M7 should need fewer cycles than an M4 for the same work rather
than more, so if the version is not a term then the two versions will agree closely and the 223
against 313 stays unexplained by it. **Predicted: the two agree within 10 per cent.** *Refuted
if* they differ by more, in which case the version is a real term with a measured size and the
chapter states how much of the gap it accounts for.

**Arm C, Zephyr 3.0.0 against 4.5.0-rc1.** At matched configuration the published 524 against
this board's 185 is a factor of 2.8, which the core alone is unlikely to carry.
**Predicted: 3.0.0 lands above 4.5.0-rc1 by a visible margin.** *Refuted if* the two agree within
10 per cent, which would place the whole factor on the core and on the two releases this bench
cannot reach, and would make arm E the only remaining explanation and permanently out of reach.

**Arm D, the laboratory's suite.** Built at matched configuration, its context switch figure on
this board is predicted to land **within a factor of 1.5 of the kernel's own benchmark's 188,
so between 125 and 282 counts**, which is the band the second criterion already uses. *Refuted
if* it falls outside, which would mean the laboratory's harness measures a different span from
the kernel's own, that the published 524 was never comparable with either 345.5 or 185, and that
the eighth criterion has been comparing harnesses rather than kernels. That is the outcome which
would retract part of the criterion rather than settle it, and it is written here before the run
so that it cannot be reinterpreted afterwards.

## What this page corrects in the chapter

Three things, all in the commit this page arrives in.

1. The roadmap named one run that cannot be built. It now names the arms above, says which are
   reachable, and says that the core candidate is not.
2. The eighth criterion named three candidate reasons. There are four: the per-switch safety
   configuration differs between the two arms and has been measured on one side only.
3. The criterion reported three directions reproduced without saying that the switch direction
   depends on a configuration choice, and reverses at 0.59 when the two arms are matched.

No measured number changes. The figures of Friday 9 October 2026 and Saturday 17 October 2026
stand exactly as they are.

## When a build file for arm D lands

This volume pins every external source to a full revision that the build compares against, and
`scripts/check_vendoring.py` refuses a build file that clones one without calling
`scripts/check_pin.py`. The suite of arm D is an external source like the vendor driver of P05,
so its build file carries a URL, a commit and the call, and the clone stays where it lies.

## What arm C must settle before a second workspace is built

Written on Friday 6 November 2026, with nothing measured and no board involved. Arm C is now
first in the order because the published 2.35 dissolved on the Zephyr side, so it is worth asking
what it costs before paying for it. The answer is not the workspace. **It is whether the harness
can be built against 3.0.0 without becoming a different harness**, which is the ground the
originally proposed run was refused on: `sections/z04.tex` rejects that run partly because "it
also moved the harness and the kernel version together, which this volume refuses elsewhere". Arm
C inherits that objection unless the line between an allowed change and a disallowed one is drawn
first.

### The rule this page adopts for arm C

**An include path or a Kconfig spelling may change. A line inside a bracket may not.**

The reason is what each kind of change can do to a number. The cold pass calls
`SCB_InvalidateICache()`, `__DSB()` and `__ISB()`, and reads `SCB->CCR` with `SCB_CCR_IC_Msk` and
`SCB_CCR_DC_Msk`. **None of those six is Zephyr's.** They are CMSIS-Core definitions from ARM,
they compile to the same instructions whichever Zephyr includes them, and the header that reaches
them is not in any bracket. Changing how that header is spelled changes no measured quantity and
can be verified by eye. Changing a `k_*` call, or what one of them does, changes the thing being
measured, and arm C is then two changes wide and is refused.

### The hazard is a false line, not a failed build

`main.c` prints its own provenance with `IS_ENABLED(CONFIG_ICACHE)` and
`IS_ENABLED(CONFIG_DCACHE)`, beside what the control register actually reports. **`IS_ENABLED` on
a Kconfig symbol that does not exist is 0, with no diagnostic from the preprocessor or the build.**
So a symbol renamed between the two releases does not stop the build. It prints
`the build asked for CONFIG_ICACHE n` under a hardware line reading `instruction cache ON`, and a
reader comparing two captures would see the caches differ between the releases when only the
spelling did. That is the direction this project has been caught by twice already, a guard or a
label asserting something the hardware contradicts, and it is why the table below lists every
symbol the harness names rather than only the ones that would fail to compile.

### Every Zephyr-facing name the harness uses, with a prediction written before the check

Taken from `zephyr/src/main.c`, `zephyr/src/mutex_case.c`, `zephyr/src/port.h` and
`adapter/`, which is the whole of this application's surface. **Predicted** is what this page
expects at tag v3.0.0 and is recorded before anything was read, so the check can refute it.

| name | where it is used | predicted at v3.0.0 |
|---|---|---|
| `zephyr/kernel.h` | every file | present |
| `zephyr/sys/util.h` | `main.c` | present |
| `zephyr/drivers/gpio.h` | `main.c`, the marker pin | present |
| `zephyr/timing/timing.h` | `main.c`, the stopwatch | present |
| `cmsis_core.h` | `main.c`, the cold pass and the cache report | **absent**, the unified spelling is later; the arch-specific path is expected instead |
| `timing_init`, `timing_start`, `timing_counter_get`, `timing_freq_get` | the stopwatch | present |
| `k_cycle_get_32`, `sys_clock_hw_cycles_per_sec` | the second stopwatch | present |
| `k_sem_take`, `k_sem_give`, `k_mutex_lock`, `k_mutex_unlock` | inside brackets | present and unchanged |
| `k_yield`, `k_thread_suspend`, `k_thread_resume`, `k_current_get` | inside brackets | present and unchanged |
| `k_thread_create`, `k_thread_priority_set`, `k_msleep`, `k_uptime_get` | setup, outside brackets | present and unchanged |
| `k_poll`, `k_poll_event`, `k_poll_event_init` | the several-objects row | present and unchanged |
| `k_work`, `k_work_submit` | the deferred row | present and unchanged |
| `DT_PATH`, `DT_GPIO_CTLR` | the marker pin | present |
| `DT_NODE_FULL_NAME` | the provenance line | present, and this is the least certain of the macros |
| `CONFIG_POLL`, `CONFIG_ASSERT`, `CONFIG_TIMESLICING`, `CONFIG_TIMESLICE_SIZE` | `prj.conf` and the provenance lines | present |
| `CONFIG_HW_STACK_PROTECTION`, `CONFIG_ARM_MPU`, `CONFIG_FPU_SHARING`, `CONFIG_PM` | the provenance lines | present |
| `CONFIG_TIMING_FUNCTIONS` | `prj.conf` | present |
| `CONFIG_ICACHE`, `CONFIG_DCACHE` | the cache report | **absent**, and this is the one that goes wrong quietly rather than loudly |
| `CONFIG_BOARD_NUCLEO_H7A3ZI_Q` | the provenance line | present, since the board is present at this tag |

*Refuted if* any name used **inside a bracket** is absent at v3.0.0 or has changed what it does,
in which case arm C moves the harness and the kernel version together, it is refused on the same
ground as the run it replaced, and this page says so rather than running it anyway.

*Also refuted if* the two predicted absences turn out to be present, which would mean the
preparation below is unnecessary and the arm is cheaper than this section claims.

### What the check is, and it needs no toolchain

The Zephyr clone on the demo laptop is at v4.5.0-rc1. With its tags fetched, every row above is a
question about one tag's tree and one tag's Kconfig, answered by `git cat-file` and `git grep`
against `v3.0.0` without configuring or building anything. Only after the table is settled is a
second workspace worth creating, and only then does the development kit of that era matter.

**And one outcome would stop arm C here**, which is worth naming so the section can end badly:
if the stopwatch itself is not available at 3.0.0 in the form this harness uses, then the two
releases cannot be measured with one instrument, and that is the same defect as comparing two
kernels with two stopwatches. The project already knows what that costs; criterion 2 failed by a
factor of five to seven for exactly that reason until Friday 9 October 2026.

## Sources

- This project's own `RESULTS.md` and `CRITERION2.md`, for every measured figure above, including
  the guard measurement of Friday 9 October 2026 at 21:15 and the agreement to 0.98 with the
  kernel's own benchmark.
- This project's `freertos/board/FreeRTOSConfig.h`, read on Monday 26 October 2026, for
  `configCHECK_FOR_STACK_OVERFLOW` at 2 on the board build, and `freertos/Makefile` for the
  kernel pin that arm B states on the command line.
- The Zephyr tree at tags v2.6.0, v2.7.0 and v3.0.0, read on Monday 26 October 2026 for the
  presence of the board and of the part.
- `methodpark/zephyr-rtosbench`, BSD-3-Clause, and `gchamp20/RTOSBench`, read on Monday 26
  October 2026 for the port that exists, the port that does not, and the two files a new port
  must provide.
- UL Solutions, Measuring real-time operating system performance, part one and part two,
  published Thursday 22 September 2022, for the published figures and for the account of porting
  the suite to each kernel. The whole published table is in this chapter's sources.
