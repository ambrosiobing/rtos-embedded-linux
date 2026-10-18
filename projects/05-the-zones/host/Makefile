# projects/05-the-zones/host/Makefile: the reduction's suite, and the proof it can fail.
#
#   make check      build and run the suite
#   make mutants    break the reduction four ways and confirm the suite notices each
#   make clean
#
# WHY A HOST SUITE EXISTS FOR THIS PROJECT AT ALL. The reduction from sixty-four zones to one
# distance is plain C over two arrays, so every answer it can give is arithmetic and none of them
# needs the board. The driver around it needs hardware; this does not, and the part of the chapter
# that can be proven without a bench should be.
#
# `make mutants` IS NOT DECORATION. A suite that has only ever passed is a suite nobody has
# checked, and this volume has the lesson on file from three counters that read zero against 949
# malformed rows. Each mutation below is a one-line edit to a copy of the source, never to the
# source itself, and the target fails if the suite still passes.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
SRCDIR  := ../src
BUILD   := build

SUITE   := $(BUILD)/test_reduce_nearest_valid
OBJSRC  := $(SRCDIR)/reduce_nearest_valid.c
TESTSRC := test_reduce_nearest_valid.c

.PHONY: all check mutants clean

all: check

$(BUILD):
	@mkdir -p $(BUILD)

$(SUITE): $(TESTSRC) $(OBJSRC) $(SRCDIR)/reduce_nearest_valid.h $(SRCDIR)/zone_status.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $(TESTSRC) $(OBJSRC)

check: $(SUITE)
	@./$(SUITE)

# Each entry is a name and the sed expression that breaks the reduction that way. The names say
# what the defect would be, because a mutation nobody can describe proves nothing about the suite.
#
#   accept_everything   the status check stops rejecting, so a zone the vendor does not call a
#                       reading can win. The grid of all status 6 then yields a distance
#   ties_to_higher      a tie picks the higher zone index instead of the lower, which makes the
#                       answer depend on loop order rather than on a stated rule
#   never_initialises   the first valid zone no longer seeds the comparison, so nothing further
#                       than zero millimetres is ever chosen
#   always_found        a result is reported as a reading before any zone is examined
MUTANTS := accept_everything ties_to_higher never_initialises always_found

SED_accept_everything := s/!zone_status_is_ranging_ok(status\[i\])/false/
SED_ties_to_higher    := s/distance_mm\[i\] < out\.distance_mm/distance_mm[i] <= out.distance_mm/
SED_never_initialises := s/!out\.found || //
SED_always_found      := s/out\.found = false;/out.found = true;/

# ONE TARGET PER MUTATION, so that make expands the sed expression rather than a shell trying to
# read a make variable. The first version of this looped in the shell over $(MUTANTS) and asked
# for $$SED_$$m, which the shell has never heard of: every expression came out empty, every
# mutation would have been a no-op, and the target would have reported that the suite was fine.
# A harness that cannot fail is the thing this target exists to rule out.
mutants: $(addprefix mutant-,$(MUTANTS))
	@echo
	@echo "all $(words $(MUTANTS)) mutations were caught, so the suite can fail"

.PHONY: $(addprefix mutant-,$(MUTANTS))

mutant-%: | $(BUILD)
	@sed '$(SED_$*)' $(OBJSRC) > $(BUILD)/mutant_$*.c
	@if cmp -s $(BUILD)/mutant_$*.c $(OBJSRC); then \
	  echo "$*: THE MUTATION DID NOT APPLY, so it proves neither direction"; exit 1; \
	fi
	@if ! $(CC) $(CFLAGS) -I$(SRCDIR) -o $(BUILD)/mutant_$* $(TESTSRC) \
	     $(BUILD)/mutant_$*.c 2>$(BUILD)/mutant_$*.log; then \
	  echo "$*: the mutant did not compile, which is not the same as being caught"; \
	  sed -n '1,5p' $(BUILD)/mutant_$*.log; exit 1; \
	fi
	@if ./$(BUILD)/mutant_$* >$(BUILD)/mutant_$*.out 2>&1; then \
	  echo "$*: NOT CAUGHT, the suite passed a reduction that is wrong"; exit 1; \
	else \
	  echo "$*: caught, $$(grep -c FAILED $(BUILD)/mutant_$*.out) check(s) failed"; \
	fi

clean:
	@rm -rf $(BUILD)
