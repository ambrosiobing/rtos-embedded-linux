# projects/01-presence/freertos/Makefile: the adapter on a host, with a real kernel.
#
# The FreeRTOS kernel is NOT vendored into this repository. It is cloned at a pinned
# tag, which keeps a published portfolio free of ten thousand lines nobody here
# wrote, and makes the version part of the build rather than part of the history.
#
#   make fetch     clone the kernel at the pinned tag, if it is not already there
#   make           build
#   make run       build and run, which is what CI does
#   make clean
#
# Point FREERTOS_KERNEL somewhere else if a clone already exists:
#   make FREERTOS_KERNEL=$(HOME)/src/FreeRTOS-Kernel run
#
# WHAT THE FIRST BUILD CORRECTED, on Tuesday 6 October 2026. The source list was
# flagged here as the uncertain part and it was right: port.c and utils/wait_for_event.c
# are the two files, and both reached their own bodies. What was wrong was the
# dialect. -std=c11 is strict ISO C, under which glibc hides every POSIX declaration,
# so pthread_sigmask, sigaction, clock_gettime, usleep, CLOCK_MONOTONIC and
# PTHREAD_STACK_MIN were all undeclared in a port whose whole job is to use them.
#
# Hence three sets of flags below rather than two, and the split is worth keeping
# rather than passing gnu11 to everything:
#
#   the kernel and the port   gnu11, POSIX visible, and not held to this project's
#                             warnings, because a warning in somebody else's tree is
#                             not this project's finding
#   the adapter               gnu11 as well, because including FreeRTOS.h on this
#                             port drags in pthread.h, plus the full wall
#   ../c/presence.c           strict c11, unchanged, because the claim this volume
#                             makes about the table is that it needs nothing beyond
#                             C11, and compiling it as gnu11 here to spare one line
#                             of Makefile would quietly retire that claim

FREERTOS_KERNEL ?= $(HOME)/src/FreeRTOS-Kernel
FREERTOS_TAG    ?= V11.1.0
FREERTOS_URL    ?= https://github.com/FreeRTOS/FreeRTOS-Kernel.git

PORT_DIR := $(FREERTOS_KERNEL)/portable/ThirdParty/GCC/Posix
CORE_SRC := $(FREERTOS_KERNEL)/tasks.c \
            $(FREERTOS_KERNEL)/queue.c \
            $(FREERTOS_KERNEL)/list.c \
            $(FREERTOS_KERNEL)/timers.c
PORT_SRC := $(PORT_DIR)/port.c \
            $(PORT_DIR)/utils/wait_for_event.c

ADAPTER_SRC := presence_adapter.c host_main.c ../adapter/phases.c
TABLE_SRC   := ../c/presence.c

INCLUDES := -I. -I../c -I../adapter -I$(FREERTOS_KERNEL)/include -I$(PORT_DIR) -I$(PORT_DIR)/utils

WALL := -Wall -Wextra -Werror -Wpedantic -Wconversion

# The adapter. gnu11 with _GNU_SOURCE, because FreeRTOS.h on this port includes
# pthread.h, and the same wall the rest of the project is built behind. -Wconversion
# is the one that catches a tick count quietly narrowed to a uint16_t.
ADAPTER_CFLAGS := -std=gnu11 -D_GNU_SOURCE -O2 -g $(WALL) $(INCLUDES)

# The table, at strict C11 and nothing else, which is the claim.
TABLE_CFLAGS := -std=c11 -O2 -g $(WALL) $(INCLUDES)

# The kernel and the port. POSIX visible through gnu11 alone, and not held to this
# project's warnings. _GNU_SOURCE is deliberately NOT passed here: port.c defines
# __USE_GNU itself, and passing both made the first build warn that it was redefined.
# The adapter does not define it, so it still gets the macro.
KERNEL_CFLAGS := -std=gnu11 -O2 -g $(INCLUDES)

LDFLAGS := -pthread

BUILD := build
BIN   := $(BUILD)/presence_freertos

.PHONY: all run fetch clean

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

$(BIN): | $(BUILD)
	@test -d "$(FREERTOS_KERNEL)" || { echo "no kernel at $(FREERTOS_KERNEL); run: make fetch"; exit 1; }
	$(CC) $(KERNEL_CFLAGS) -c $(CORE_SRC) $(PORT_SRC)
	mv *.o $(BUILD)/
	$(CC) $(ADAPTER_CFLAGS) -c $(ADAPTER_SRC)
	mv *.o $(BUILD)/
	$(CC) $(TABLE_CFLAGS) -c $(TABLE_SRC)
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

clean:
	rm -rf $(BUILD)
