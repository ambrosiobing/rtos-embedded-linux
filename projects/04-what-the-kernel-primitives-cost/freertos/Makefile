# projects/04-what-the-kernel-primitives-cost/freertos/Makefile: the adapter on a host and on
# the board, with a real kernel in both cases.
#
# NOTHING IS VENDORED. The FreeRTOS kernel is cloned at a pinned tag, as P01 and P02 clone it,
# and the board support package is the sibling firmware volume at a pinned commit, cloned the
# same way. Both keep tens of thousands of lines nobody here wrote out of a published
# repository, and both make the version part of the build rather than part of the history.
#
#   make fetch       clone the kernel at the pinned tag, if it is not already there
#   make run         the host build, run; the campaign's own verdicts set the exit status
#   make check       the host build, run, then prove the period capture is refused for its clock
#   make fetch-bsp   clone the sibling volume at the pinned commit, if it is not already there
#   make board       the board build: an .elf and a .bin for the NUCLEO-H7A3ZI-Q
#   make clean
#
# THE HOST HALF IS P02'S, flag for flag. Three sets of flags, and the split is worth keeping:
# the kernel and the port at gnu11 and not held to this project's warnings, the adapter at
# gnu11 with the full wall, and the core at strict C11, because the claim about ../adapter is
# that it needs no kernel, no vendor header and nothing beyond C11.
#
# THE BOARD HALF NEEDS arm-none-eabi-gcc, the GNU toolchain for Cortex-M with newlib, which is
# what the sibling volume's retarget.c implements the hooks for. In WSL on the demo laptop that
# is one `sudo apt install gcc-arm-none-eabi`; the Zephyr SDK's compiler is a GCC too but ships
# picolibc, whose stdio is wired differently, so it is not used here. Override CROSS to point
# at another prefix.

FREERTOS_KERNEL ?= $(HOME)/src/FreeRTOS-Kernel
FREERTOS_TAG    ?= V11.1.0
FREERTOS_URL    ?= https://github.com/FreeRTOS/FreeRTOS-Kernel.git

NUCLEO_BSP        ?= $(HOME)/src/firmware-nucleo-h7
NUCLEO_BSP_COMMIT ?= 1d6d10f826ee0728ded6533defc468baa2ff0f2a
NUCLEO_BSP_URL    ?= https://github.com/ambrosiobing/firmware-nucleo-h7.git

CROSS ?= arm-none-eabi-

WALL := -Wall -Wextra -Werror -Wpedantic -Wconversion

# =============================================== the pins are checked, and not only written down
#
# WHY THIS RUNS ON A BUILD AND NOT ONLY ON A FETCH TARGET. `fetch` clones the kernel at the tag
# when the directory is absent and leaves it alone when it is not. `fetch-bsp` went one better
# and printed a NOTE when the board support clone was off its pin, but a note is not a refusal and
# it only appeared when that target was run, never on `make board`. So until Monday 19 October
# 2026 the two revisions above were statements of intent rather than properties of the image, and
# every published figure in docs/RESULTS.md is attributed to them. P05 was fixed first, in cmake,
# and Joseph decided one rule should apply across the set.
#
# THIS PROJECT HAS TWO PINS AND THEY ARE DIFFERENT PROMISES. The kernel is pinned by release tag,
# which is the upstream's own convention; the sibling volume is pinned by a whole commit, which
# cannot be moved by anyone. scripts/check_pin.py carries that distinction so these Makefiles do
# not have to argue it three times.
#
# ORDER-ONLY, so the check runs before a build and does not make the binary out of date. A phony
# prerequisite in the ordinary position would relink on every invocation.
PYTHON    ?= python3
REPO_ROOT := ../../..

# Build against whatever is checked out, deliberately and loudly: make ALLOW_ANY_REVISION=1 board
ALLOW_ANY_REVISION ?=
PIN_ANY := $(if $(ALLOW_ANY_REVISION),--allow-any,)

.PHONY: all run check fetch fetch-bsp board check-pins check-pins-bsp clean

check-pins:
	@$(PYTHON) $(REPO_ROOT)/scripts/check_pin.py --path "$(FREERTOS_KERNEL)" \
	  --tag $(FREERTOS_TAG) --url $(FREERTOS_URL) --what "the FreeRTOS kernel" $(PIN_ANY)

# The board support package is only needed by the board build, so it is a separate target rather
# than part of check-pins: a host run must not be refused for a clone it never reads.
check-pins-bsp:
	@$(PYTHON) $(REPO_ROOT)/scripts/check_pin.py --path "$(NUCLEO_BSP)" \
	  --commit $(NUCLEO_BSP_COMMIT) --url $(NUCLEO_BSP_URL) \
	  --what "the board support package" $(PIN_ANY)

# ============================================================================ the host build

POSIX_PORT_DIR := $(FREERTOS_KERNEL)/portable/ThirdParty/GCC/Posix
KERNEL_SRC := $(FREERTOS_KERNEL)/tasks.c \
              $(FREERTOS_KERNEL)/queue.c \
              $(FREERTOS_KERNEL)/list.c \
              $(FREERTOS_KERNEL)/timers.c \
              $(FREERTOS_KERNEL)/event_groups.c
POSIX_PORT_SRC := $(POSIX_PORT_DIR)/port.c \
                  $(POSIX_PORT_DIR)/utils/wait_for_event.c

HOST_ADAPTER_SRC := measure_adapter.c mutex_case.c host_port.c host_main.c
CORE_SRC         := ../adapter/measure.c ../adapter/campaign.c
HEADERS          := FreeRTOSConfig.h mutex_case.h port.h ../adapter/measure.h ../adapter/campaign.h

HOST_INCLUDES := -I. -I../adapter -I$(FREERTOS_KERNEL)/include -I$(POSIX_PORT_DIR) -I$(POSIX_PORT_DIR)/utils

# Two hundred and fifty times the board's counts. The first run at twenty-five passed with a
# ratio of 2.2 against a threshold of 2, and the dilution arithmetic explains the thinness: the
# work predicts five, the port's switching cost sits in both arms, and at a 15 us spin against
# 39 us of switching, (5 x 15 + 39) / (15 + 39) is 2.1. Ten times the spin predicts about 4.2.
HOST_SPIN := -DHOLD_SPIN=500000u -DMEDIUM_SPIN=2000000u

HOST_ADAPTER_CFLAGS := -std=gnu11 -D_GNU_SOURCE -O2 -g $(WALL) $(HOST_INCLUDES) $(HOST_SPIN)
HOST_CORE_CFLAGS    := -std=c11 -O2 -g $(WALL) $(HOST_INCLUDES)
HOST_KERNEL_CFLAGS  := -std=gnu11 -O2 -g $(HOST_INCLUDES)

BUILD := build
BIN   := $(BUILD)/measure_freertos

all: $(BIN)

fetch:
	@if [ ! -d "$(FREERTOS_KERNEL)" ]; then \
	  echo "cloning $(FREERTOS_URL) at $(FREERTOS_TAG)"; \
	  git clone --depth 1 --branch $(FREERTOS_TAG) $(FREERTOS_URL) "$(FREERTOS_KERNEL)"; \
	else \
	  echo "$(FREERTOS_KERNEL) is already present, leaving it alone"; \
	fi
	@echo "kernel version in the clone:"
	@git -C "$(FREERTOS_KERNEL)" describe --tags 2>/dev/null || echo "  no tag reported"

$(BUILD):
	mkdir -p $(BUILD)

# Every source and header is a prerequisite, which P01 and P02 learned the expensive way: a
# rule that depended on the build directory alone never rebuilt, and a stale binary printed a
# pass.
$(BIN): $(HOST_ADAPTER_SRC) $(CORE_SRC) $(HEADERS) Makefile | $(BUILD) check-pins
	@test -d "$(FREERTOS_KERNEL)" || { echo "no kernel at $(FREERTOS_KERNEL); run: make fetch"; exit 1; }
	$(CC) $(HOST_KERNEL_CFLAGS) -c $(KERNEL_SRC) $(POSIX_PORT_SRC)
	mv *.o $(BUILD)/
	$(CC) $(HOST_ADAPTER_CFLAGS) -c $(HOST_ADAPTER_SRC)
	mv *.o $(BUILD)/
	$(CC) $(HOST_CORE_CFLAGS) -c $(CORE_SRC)
	mv *.o $(BUILD)/
	$(CC) $(BUILD)/*.o -pthread -o $(BIN)
	@echo
	@echo "no allocator is linked, which is what configSUPPORT_DYNAMIC_ALLOCATION 0 means:"
	@if nm $(BIN) | grep -qE " T (pvPortMalloc|vPortFree)$$"; then \
	  echo "  FAIL pvPortMalloc or vPortFree is in the binary"; exit 1; \
	else \
	  echo "  pvPortMalloc and vPortFree are absent"; \
	fi

run: $(BIN)
	./$(BIN)

# EVERY CAPTURE THE HOST BUILD EMITS MUST BE REFUSED, and that is checked rather than claimed.
# The period capture is cut from the run's output, the witness fields it would need are
# appended so that the only thing left to refuse it for is its clock, and the reduction is
# asked. A host build that could get a capture past the reduction would be the thing the
# contract forbids.
check: $(BIN)
	./$(BIN) > $(BUILD)/run.txt; status=$$?; \
	if [ $$status -ne 0 ]; then cat $(BUILD)/run.txt; exit $$status; fi
	@sed -n '/^# verdicts/,$$p' $(BUILD)/run.txt
	@awk '/^instrument wall/{p=1} p{print} p&&/^a_counts/{exit}' $(BUILD)/run.txt > $(BUILD)/device.txt
	@test -s $(BUILD)/device.txt || { echo "  FAIL no period capture in the run output"; exit 1; }
	@printf 'resolution_s 0.00001\nb_edges_s' > $(BUILD)/witness.txt
	@i=0; while [ $$i -le 64 ]; do printf ' %d.%03d' $$((i/1000)) $$((i%1000)) >> $(BUILD)/witness.txt; i=$$((i+1)); done
	@printf '\n' >> $(BUILD)/witness.txt
	@cat $(BUILD)/device.txt $(BUILD)/witness.txt > $(BUILD)/whole.txt
	@echo
	@echo "the period capture, fed to the reduction with a witness half attached:"
	@if python3 ../host/reduce.py $(BUILD)/whole.txt > $(BUILD)/reduce.txt 2>&1; then \
	  echo "  FAIL the reduction accepted a capture from a host build"; cat $(BUILD)/reduce.txt; exit 1; \
	elif grep -q 'clock_hz' $(BUILD)/reduce.txt; then \
	  echo "  refused for its clock, as the contract requires:"; sed 's/^/    /' $(BUILD)/reduce.txt; \
	else \
	  echo "  FAIL refused, but not for its clock:"; cat $(BUILD)/reduce.txt; exit 1; \
	fi

# =========================================================================== the board build

XCC     := $(CROSS)gcc
XOBJCPY := $(CROSS)objcopy
XSIZE   := $(CROSS)size
XNM     := $(CROSS)nm

CM7_PORT_DIR := $(FREERTOS_KERNEL)/portable/GCC/ARM_CM7/r0p1
CM7_PORT_SRC := $(CM7_PORT_DIR)/port.c

# THE SIBLING VOLUME'S BOARD SUPPORT, the nine files its own firmware links. P01 of that volume
# owns them and every project there links the same set; this one does too.
BSP_SRC := $(NUCLEO_BSP)/c/board/startup.c \
           $(NUCLEO_BSP)/c/board/system.c \
           $(NUCLEO_BSP)/c/board/board.c \
           $(NUCLEO_BSP)/c/board/uart.c \
           $(NUCLEO_BSP)/c/board/cycles.c \
           $(NUCLEO_BSP)/c/board/icache.c \
           $(NUCLEO_BSP)/c/board/clock280.c \
           $(NUCLEO_BSP)/c/board/retarget.c \
           $(NUCLEO_BSP)/c/clock/clocktree.c
BSP_LD  := $(NUCLEO_BSP)/c/ld/stm32h7a3zi.ld

BOARD_ADAPTER_SRC := measure_adapter.c mutex_case.c board_port.c board_main.c
BOARD_HEADERS     := board/FreeRTOSConfig.h mutex_case.h port.h ../adapter/measure.h ../adapter/campaign.h

# The part, as the sibling's toolchain file states it: Cortex-M7 with the double precision
# unit and the hardware calling convention. fpv5-d16 and not fpv5-sp-d16, because this part
# has the double precision unit and the single precision flag would push double arithmetic into
# software. The FreeRTOS port refuses to compile without hardware floating point at all.
MCU := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# board/ first, so the board's FreeRTOSConfig.h is the one the kernel sees and the host's never
# is. -Os as the sibling's release configuration, because -O0 on this core produces code whose
# timing is unrepresentative of anything shippable.
BOARD_INCLUDES := -Iboard -I. -I../adapter -I$(FREERTOS_KERNEL)/include -I$(CM7_PORT_DIR) \
                  -I$(NUCLEO_BSP)/c/board -I$(NUCLEO_BSP)/c/clock
BOARD_COMMON   := $(MCU) -Os -g -ffunction-sections -fdata-sections -fno-common

BOARD_ADAPTER_CFLAGS := -std=gnu11 $(BOARD_COMMON) $(WALL) $(BOARD_INCLUDES)
BOARD_CORE_CFLAGS    := -std=c11 $(BOARD_COMMON) $(WALL) $(BOARD_INCLUDES)
BOARD_KERNEL_CFLAGS  := -std=gnu11 $(BOARD_COMMON) $(BOARD_INCLUDES)
BOARD_BSP_CFLAGS     := -std=c11 $(BOARD_COMMON) -I$(NUCLEO_BSP)/c/board -I$(NUCLEO_BSP)/c/clock

BOARD_BUILD := $(BUILD)/board
BOARD_ELF   := $(BOARD_BUILD)/measure_freertos.elf
BOARD_BIN   := $(BOARD_BUILD)/measure_freertos.bin

# nano.specs and nosys.specs as the sibling links: the reduced newlib, with its syscalls
# satisfied by retarget.c first and by nosys for anything retarget.c does not define. The map
# file is written because the no-allocator check reads the binary and a reader reads the map.
BOARD_LDFLAGS := $(MCU) -T$(BSP_LD) -Wl,--gc-sections -Wl,--print-memory-usage \
                 -Wl,-Map=$(BOARD_BUILD)/measure_freertos.map \
                 -specs=nano.specs -specs=nosys.specs

fetch-bsp:
	@if [ ! -d "$(NUCLEO_BSP)" ]; then \
	  echo "cloning $(NUCLEO_BSP_URL)"; \
	  git clone $(NUCLEO_BSP_URL) "$(NUCLEO_BSP)"; \
	  git -C "$(NUCLEO_BSP)" checkout --quiet $(NUCLEO_BSP_COMMIT); \
	else \
	  echo "$(NUCLEO_BSP) is already present, leaving it alone"; \
	fi
	@echo "board support at commit:"
	@git -C "$(NUCLEO_BSP)" rev-parse HEAD
#	THE COMPARISON IS AGAINST THE WHOLE HASH, and it used to be against `rev-parse --short`,
#	which was correct only while the pin above was also abbreviated. Widening the pin to all
#	forty characters on Sunday 18 October 2026 would have left a seven-character answer compared
#	against a forty-character pin, so the note would have fired on every run including the
#	correct one. A comparison that can only report a mismatch is worse than none, because the
#	first few false alarms teach a reader to skip the line.
	@if [ "$$(git -C "$(NUCLEO_BSP)" rev-parse HEAD)" != "$(NUCLEO_BSP_COMMIT)" ]; then \
	  echo "  NOTE that is not the pinned $(NUCLEO_BSP_COMMIT); the build uses what is checked out"; \
	fi

$(BOARD_BUILD):
	mkdir -p $(BOARD_BUILD)

board: $(BOARD_BIN)

$(BOARD_ELF): $(BOARD_ADAPTER_SRC) $(CORE_SRC) $(BOARD_HEADERS) Makefile | $(BOARD_BUILD) check-pins check-pins-bsp
	@test -d "$(FREERTOS_KERNEL)" || { echo "no kernel at $(FREERTOS_KERNEL); run: make fetch"; exit 1; }
	@test -d "$(NUCLEO_BSP)" || { echo "no board support at $(NUCLEO_BSP); run: make fetch-bsp"; exit 1; }
	@command -v $(XCC) > /dev/null || { echo "no $(XCC) on PATH; in WSL: sudo apt install gcc-arm-none-eabi"; exit 1; }
	$(XCC) $(BOARD_KERNEL_CFLAGS) -c $(KERNEL_SRC) $(CM7_PORT_SRC)
	mv *.o $(BOARD_BUILD)/
	$(XCC) $(BOARD_BSP_CFLAGS) -c $(BSP_SRC)
	mv *.o $(BOARD_BUILD)/
	$(XCC) $(BOARD_ADAPTER_CFLAGS) -c $(BOARD_ADAPTER_SRC)
	mv *.o $(BOARD_BUILD)/
	$(XCC) $(BOARD_CORE_CFLAGS) -c $(CORE_SRC)
	mv *.o $(BOARD_BUILD)/
	$(XCC) $(BOARD_LDFLAGS) $(BOARD_BUILD)/*.o -o $(BOARD_ELF)
	@echo
	@echo "no allocator is linked, which is what configSUPPORT_DYNAMIC_ALLOCATION 0 means:"
	@if $(XNM) $(BOARD_ELF) | grep -qE " T (pvPortMalloc|vPortFree)$$"; then \
	  echo "  FAIL pvPortMalloc or vPortFree is in the image"; exit 1; \
	else \
	  echo "  pvPortMalloc and vPortFree are absent"; \
	fi
	@$(XSIZE) $(BOARD_ELF)

$(BOARD_BIN): $(BOARD_ELF)
	$(XOBJCPY) -O binary $(BOARD_ELF) $(BOARD_BIN)
	@echo "image: $(BOARD_BIN), $$(stat -c%s $(BOARD_BIN)) bytes"

clean:
	rm -rf $(BUILD)
