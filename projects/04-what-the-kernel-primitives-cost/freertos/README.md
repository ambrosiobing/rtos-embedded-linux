# The seven operations under FreeRTOS, on a host

**Status: green in WSL on the demo laptop, Saturday 17 October 2026, first build.** No
allocator in the binary, all seven verdicts passed, and the reduction refused the period
capture for its clock and for nothing else. No board was involved and none is wired for this:
the POSIX port runs the kernel as a host process.

The first version of `make check` hid the verdict lines on success and showed only the
refusal, so a green run concealed its own evidence. It prints them now.

The map from each row's "what happens" to this kernel's objects is in
[../docs/DESIGN.md](../docs/DESIGN.md), and it was committed before this directory existed.
That order is the point: a map written after the code describes the code.

## What a green run proves, and what it cannot

**Every capture this build emits is refused by the reduction**, and `make check` proves it
rather than asserting it. The POSIX port has no cycle counter; `measure_now()` reads the
monotonic clock in hundreds of nanoseconds so that brackets come back nonzero and in order,
and `measure_clock_hz()` returns zero, so every capture carries `clock_hz 0`. No figure from
this directory goes near [../docs/RESULTS.md](../docs/RESULTS.md).

What a green run does show, under a real kernel and with no board:

- all seven operations complete, which the first Zephyr run on the board did not, when an
  always-ready partner at the measurer's priority starved the giver below it
- no bracket returns zero or wraps
- **the inheriting arm of criterion 4 waits for less than the non-inheriting arm**, which is
  the one row with a built-in refutation. A mutex that did not inherit would put both arms at
  the larger figure, and the verdict would fail

## What is the same as the Zephyr adapter, and what is not

The same three priorities in the same order, numbered the other way up because FreeRTOS counts
upward. The same partner created suspended and resumed only around the yield row. The same
giver stamping as its last statement before the give. The same three-task contended case with
the holder announcing before its work.

Two rows use a different object and say so. The several-objects row waits on an **event
group** rather than polling two semaphores, so in both kernels it prices "the alternative to
one object" rather than "the same object waited on differently". Queue sets were considered
and not used: whether the pinned V11.1.0 has a static creator for them is not established, and
this build links no allocator. The hand-work-to-a-queue row uses `xTimerPendFunctionCall` to
the timer daemon task, which sits above the measurer so the post preempts into the handler and
the bracket holds one switch, as Zephyr's system work queue does.

**The mutex comparison keeps its shape.** A FreeRTOS mutex inherits under `configUSE_MUTEXES`
and cannot be told not to, exactly as `k_mutex` cannot, so the other arm is a binary semaphore
used as a lock in both kernels: two protocols rather than two settings of one object.

**Time slicing is off and stated.** The Zephyr adapter was built with it on by default for a
day before anyone read the default; here it is off from the first line, because the yield row
runs two tasks at equal priority and a slice boundary inside a bracket would add a switch the
bracket did not ask for.

**Criterion 3 does not exist here.** A host has no instruction cache to invalidate. The board
adapter carries it.

**The contended case's spin counts are two hundred and fifty times the board's**, passed from the
Makefile, because the POSIX port switches tasks through signals at tens of microseconds and the
board's few microseconds of spin would be lost in the switching. The four-to-one ratio is kept,
and the figures are refused by the reduction regardless.

## Building it

    make fetch     clone the kernel at V11.1.0, if it is not already there
    make run       build and run; the binary's verdicts set the exit status
    make check     run, then prove the period capture is refused for its clock

The kernel is cloned at a pinned tag rather than vendored. If P01's or P02's clone is already
at `~/src/FreeRTOS-Kernel`, `make fetch` reports it and leaves it alone. The no-allocator check
runs on every build.

## The board

**Written Saturday 17 October 2026 and not yet built anywhere.** `make fetch-bsp && make board`
produces an `.elf` and a `.bin` for the NUCLEO-H7A3ZI-Q from the Cortex-M7 port at the same
V11.1.0, a second `FreeRTOSConfig.h` in `board/` with the real clock, tick and stack sizes and
stack-overflow checking on, and the sibling firmware volume's board support at a pinned commit:
its vector table, linker script, gated climb to 280 MHz, cycle counter, instruction cache and
console. None of that is copied. It is cloned, as the kernel is.

**The campaign is the same one.** On Saturday 17 October 2026 the gates, the instrument's own
cost, the warm and cold passes, criterion 1 and the verdicts moved out of the Zephyr main into
[../adapter/campaign.c](../adapter/campaign.c), which knows no kernel, and both board builds and
the host build run it with hooks for the few things that differ: how to spin and sleep, what
the hardware reports, how to drive the marker. One orchestration against each kernel, which is
P02's `phases.c` arriving here. **The proof that the lift changed nothing is the Zephyr board:
its figures after must sit within two per cent of its figures before.**

The board port chooses its two counters exactly as the Zephyr port does, because the choice is
the part's: the data watchpoint cycle counter for the six brackets that contain no idle, and a
count derived from the system tick, which runs through idle, for the one that does. The marker
is PB4, the same pin the Zephyr overlay names as `gpio@58020400` pin 4, so the lead already on
CN7 pin 19 needs no change.

**The main refuses to start the scheduler if the part is not at 280 MHz.** The kernel is
configured for that rate, the climb is the sibling's gated one, the registers are decoded
afterwards, and a decode that disagrees with the configuration halts with the step that failed
printed. A tick configured for one rate on a part running at another would make every wall
figure wrong by a factor while the console looked fine.

The toolchain is `arm-none-eabi-gcc` with newlib, which the sibling's `retarget.c` implements
the hooks for. In WSL that is one `sudo apt install gcc-arm-none-eabi`. CI links the image on
every push and never runs it; the figures come from the bench.
