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
# THE SOURCE LIST BELOW IS THE PART THAT HAS NOT BEEN CONFIRMED. The kernel's four
# core files are not in doubt; what the POSIX port needs beside port.c is, and the
# first build in WSL on the demo laptop is what settles it rather than CI. If the
# link fails for a missing symbol, the file that defines it belongs in PORT_SRC and
# nothing else here changes.

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

APP_SRC  := presence_adapter.c host_main.c ../c/presence.c

INCLUDES := -I. -I../c -I$(FREERTOS_KERNEL)/include -I$(PORT_DIR) -I$(PORT_DIR)/utils

# The same wall the rest of this project is built behind, so the adapter is held to
# the standard the table is. -Wconversion included, which is the one that catches a
# tick count quietly narrowed to a uint16_t.
CFLAGS  := -std=c11 -O2 -g -Wall -Wextra -Werror -Wpedantic -Wconversion $(INCLUDES)
LDFLAGS := -pthread

# The kernel's own sources are compiled without the project's wall. They are not this
# project's code, a warning in them is not this project's finding, and -Werror over
# somebody else's tree turns their next release into our build failure.
KERNEL_CFLAGS := -std=c11 -O2 -g $(INCLUDES)

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
	$(CC) $(CFLAGS) -c $(APP_SRC)
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
