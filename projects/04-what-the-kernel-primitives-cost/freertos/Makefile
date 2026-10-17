# projects/04-what-the-kernel-primitives-cost/freertos/Makefile: the adapter on a host, with a
# real kernel.
#
# The FreeRTOS kernel is NOT vendored into this repository. It is cloned at a pinned tag, which
# keeps a published portfolio free of ten thousand lines nobody here wrote, and makes the
# version part of the build rather than part of the history.
#
#   make fetch     clone the kernel at the pinned tag, if it is not already there
#   make           build
#   make run       build and run; the binary's own verdicts decide the exit status
#   make check     run, then prove that every capture it emitted is refused by the reduction
#   make clean
#
# THE FLAGS AND THE SOURCE LIST ARE P02'S, which is the point of a third project using the same
# kernel. Three sets of flags, and the split is worth keeping: the kernel and the port at gnu11
# and not held to this project's warnings, the adapter at gnu11 with the full wall, and the
# core at strict C11, because the claim about ../adapter/measure.c is that it needs no kernel,
# no vendor header and nothing beyond C11, and compiling it as gnu11 to spare a line here would
# quietly retire that claim.
#
# WHAT IS DIFFERENT FROM P02. event_groups.c joins the kernel sources, because the
# several-objects row waits on an event group. And the host build passes spin counts for the
# contended case twenty-five times the board's, through HOLD_SPIN and MEDIUM_SPIN, because the
# POSIX port switches tasks through signals at tens of microseconds each and the board's few
# microseconds of spin would be lost in the switching. The ratio of four to one is kept.

FREERTOS_KERNEL ?= $(HOME)/src/FreeRTOS-Kernel
FREERTOS_TAG    ?= V11.1.0
FREERTOS_URL    ?= https://github.com/FreeRTOS/FreeRTOS-Kernel.git

PORT_DIR := $(FREERTOS_KERNEL)/portable/ThirdParty/GCC/Posix
CORE_SRC := $(FREERTOS_KERNEL)/tasks.c \
            $(FREERTOS_KERNEL)/queue.c \
            $(FREERTOS_KERNEL)/list.c \
            $(FREERTOS_KERNEL)/timers.c \
            $(FREERTOS_KERNEL)/event_groups.c
PORT_SRC := $(PORT_DIR)/port.c \
            $(PORT_DIR)/utils/wait_for_event.c

ADAPTER_SRC := measure_adapter.c mutex_case.c host_main.c
CORE_C      := ../adapter/measure.c

INCLUDES := -I. -I../adapter -I$(FREERTOS_KERNEL)/include -I$(PORT_DIR) -I$(PORT_DIR)/utils

WALL := -Wall -Wextra -Werror -Wpedantic -Wconversion

# Two hundred and fifty times the board's counts, not twenty-five. The first run at twenty-five
# passed with a ratio of 2.2 against a threshold of 2, and the dilution arithmetic explains the
# thinness: the ratio the work predicts is five, the port's switching cost sits in both arms,
# and at a 15 us spin against 39 us of switching, (5 x 15 + 39) / (15 + 39) is 2.1. Ten times
# the spin predicts about 4.2, which is a pass by two rather than by a fifth. The contended arm
# then takes about fifty milliseconds for its sixty-four samples, which is nothing.
HOST_SPIN := -DHOLD_SPIN=500000u -DMEDIUM_SPIN=2000000u

ADAPTER_CFLAGS := -std=gnu11 -D_GNU_SOURCE -O2 -g $(WALL) $(INCLUDES) $(HOST_SPIN)
CORE_CFLAGS    := -std=c11 -O2 -g $(WALL) $(INCLUDES)
KERNEL_CFLAGS  := -std=gnu11 -O2 -g $(INCLUDES)

LDFLAGS := -pthread

BUILD := build
BIN   := $(BUILD)/measure_freertos

.PHONY: all run check fetch clean

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

HEADERS := FreeRTOSConfig.h mutex_case.h ../adapter/measure.h

# Every source and header is a prerequisite, which P01 and P02 learned the expensive way: a
# rule that depended on the build directory alone never rebuilt, and a stale binary printed a
# pass.
$(BIN): $(ADAPTER_SRC) $(CORE_C) $(HEADERS) Makefile | $(BUILD)
	@test -d "$(FREERTOS_KERNEL)" || { echo "no kernel at $(FREERTOS_KERNEL); run: make fetch"; exit 1; }
	$(CC) $(KERNEL_CFLAGS) -c $(CORE_SRC) $(PORT_SRC)
	mv *.o $(BUILD)/
	$(CC) $(ADAPTER_CFLAGS) -c $(ADAPTER_SRC)
	mv *.o $(BUILD)/
	$(CC) $(CORE_CFLAGS) -c $(CORE_C)
	mv *.o $(BUILD)/
	$(CC) $(BUILD)/*.o $(LDFLAGS) -o $(BIN)
	@echo
	@echo "no allocator is linked, which is what configSUPPORT_DYNAMIC_ALLOCATION 0 means:"
	@if nm $(BIN) | grep -qE " T (pvPortMalloc|vPortFree)$$"; then \
	  echo "  FAIL pvPortMalloc or vPortFree is in the binary"; exit 1; \
	else \
	  echo "  pvPortMalloc and vPortFree are absent"; \
	fi

run: $(BIN)
	./$(BIN)

# EVERY CAPTURE THIS BUILD EMITS MUST BE REFUSED, and that is checked rather than claimed. The
# period capture is cut from the run's output, the witness fields it would need are appended
# so that the only thing left to refuse it for is its clock, and the reduction is asked. A
# host build that could get a capture past the reduction would be the thing the contract
# forbids.
check: $(BIN)
	./$(BIN) > $(BUILD)/run.txt; status=$$?; \
	if [ $$status -ne 0 ]; then cat $(BUILD)/run.txt; exit $$status; fi
	@sed -n '/^# host verdicts/,$$p' $(BUILD)/run.txt
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

clean:
	rm -rf $(BUILD)
