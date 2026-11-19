# projects/05-the-zones/integration/CMakeLists.txt
#
# The subject build of criterion 6. docs/CRITERION6.md is the whole reason this directory exists:
# P01's machine is linked here unchanged, this project's reduction feeds it, and the claim is
# settled by comparing one object with the one P01's own build produces.
#
#   cmake -B build-p05-integration -GNinja -DBOARD=native_sim \
#         -S projects/05-the-zones/integration
#   ninja -C build-p05-integration
#   ./build-p05-integration/zephyr/zephyr.exe
#
# THE BOARD IS NOT A CHOICE. P01's Zephyr application is built for native_sim with the host
# compiler, and an object from a native_sim build compared against an object from a cross build
# would be x86 against ARM, which is not a comparison. So this build targets what the reference
# targets, and the comparison that follows is between two objects a reader can hold to each other.
#
# NOTHING HERE NEEDS THE PART. src/reduce_nearest_valid.c and src/zone_status.h name no operating
# system and no vendor interface, which is why they already have a host suite under ../host, so
# this build needs no board, no shield, no vendor clone and no cross toolchain.

cmake_minimum_required(VERSION 3.20.0)

# native_sim compiles with the host gcc. With no toolchain variant set, Zephyr goes looking for
# its SDK and stops when it is not there, so this says what the build needs rather than leaving it
# on the command line. An explicit setting still wins, as it does in P01's build file.
if(NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT AND NOT DEFINED ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  set(ZEPHYR_TOOLCHAIN_VARIANT host)
endif()

# THE CONFIGURATION IS P01'S OWN FILE, POINTED AT RATHER THAN COPIED, and this is the line that
# makes the criterion sound. Two applications with two prj.conf files have two generated
# autoconf.h, and while presence.c names no CONFIG_ symbol and so cannot be reached by that, the
# options there also decide the optimisation level and what the compiler is handed. A copy would
# satisfy the comparison on the day it was made and quietly stop doing so the first time one of
# the two was edited. docs/CRITERION6.md names this as one of the three ways the comparison could
# be satisfied while proving nothing.
set(CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../01-presence/zephyr/prj.conf")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zones_into_presence C)

# P01'S TWO FILES ARE TAKEN FROM WHERE THEY LIVE, not copied in. The machine and the Zephyr
# adapter are P01's, unmodified, and that is the thing being claimed. phases.c is deliberately
# absent: it is P01's four-phase test harness, and this build is a consumer rather than a test of
# the adapter.
target_sources(app PRIVATE
  main.c
  ../../01-presence/zephyr/presence_adapter.c
  ../../01-presence/c/presence.c
  ../src/reduce_nearest_valid.c
)

target_include_directories(app PRIVATE
  .
  ../src
  ../../01-presence/adapter
  ../../01-presence/c
  ../../01-presence/zephyr
)

# THE SAME STANDARD P01 HOLDS IT TO, and this line is not tidiness. P01's build file compiles this
# file with -std=c11 explicitly, so a build here that left it to the default would produce a
# different object from the same source and the criterion would report a difference that is only a
# flag. Of the three refutations docs/CRITERION6.md writes down, this is the one the two build
# files have to agree on by hand.
set_source_files_properties(../../01-presence/c/presence.c PROPERTIES COMPILE_OPTIONS "-std=c11")

# The reduction is held to the same standard, for the reason its host suite already gives: it is
# plain C, it needs no extension, and letting it drift into whichever dialect a build system
# happens to pass would retire that claim one build system at a time.
set_source_files_properties(../src/reduce_nearest_valid.c PROPERTIES COMPILE_OPTIONS "-std=c11")
