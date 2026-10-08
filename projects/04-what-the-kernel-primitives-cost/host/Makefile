# projects/04-what-the-kernel-primitives-cost/host/Makefile
#
# Builds the measurement core with a host port, so that the capture format can be exercised
# against the reduction that parses it with no board involved.
#
# THE WARNING SET IS THE VOLUME'S, not a reduced one for a host build. A core that only compiles
# cleanly under a cross compiler is a core whose host build is not a check.
#
# EVERY SOURCE AND HEADER IS A PREREQUISITE, which P01 and P02 both learned the expensive way on
# Tuesday 6 October 2026: a rule that depended on the build directory alone never rebuilt, and a
# stale binary printed a pass.

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion \
           -Wstrict-prototypes -Wmissing-prototypes
BUILD   := build
BIN     := $(BUILD)/emit_capture

SRCS    := emit_capture.c ../adapter/measure.c
HDRS    := ../adapter/measure.h

.PHONY: all check clean

all: $(BIN)

$(BIN): $(SRCS) $(HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

$(BUILD):
	mkdir -p $(BUILD)

# The loop this directory exists to close: the C emits, the Python parses, and a disagreement
# between them is a failure rather than a surprise on a bench evening.
check: $(BIN)
	python3 test_capture_pipeline.py $(BIN)
	python3 test_reduce.py

clean:
	rm -rf $(BUILD)
