# projects/02-the-claim/zephyr/CMakeLists.txt
#
# A Zephyr application, built for native_sim, which uses the HOST compiler rather than
# the Zephyr SDK. That is why this adapter needs a Zephyr tree but not the
# three gigabyte cross toolchain: nothing here is cross compiled.
#
# Built with plain CMake rather than `west build`, and that is deliberate rather than
# a preference. `west build` is an extension command that west finds through its
# workspace manifest, so it is only available from inside ~/zephyrproject; this
# repository is a separate tree. `west zephyr-export` registers Zephyr's CMake package
# in ~/.cmake/packages/Zephyr, and find_package below picks it up from anywhere, so
# the build needs no workspace and no west at all:
#
#   cmake -B build-zephyr -GNinja -DBOARD=native_sim -S projects/02-the-claim/zephyr
#   ninja -C build-zephyr
#   ./build-zephyr/zephyr/zephyr.exe
#
# The version this was built against is recorded in README.md rather than pinned
# here, because `west init` with no revision takes the manifest's main branch and
# there is no tag to pin, which is itself a difference from the FreeRTOS adapter.
#
# The four test phases are ../adapter/phases.c, shared with the FreeRTOS adapter and
# containing no kernel header, so the same test runs against both.

cmake_minimum_required(VERSION 3.20.0)

# native_sim compiles with the host gcc, but Zephyr does not assume that: with no
# toolchain variant set it goes looking for the Zephyr SDK and stops when it is not
# there. Saying "host" is the whole of what this build needs, and it is set here
# rather than left on the command line so the documented build line is the build line.
# An explicit setting from the environment or the cache still wins, because a real
# board would need one.
if(NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT AND NOT DEFINED ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  set(ZEPHYR_TOOLCHAIN_VARIANT host)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(claim_zephyr C)

target_sources(app PRIVATE
  claim_adapter.c
  main.c
  ../adapter/phases.c
  ../c/claim.c
)

target_include_directories(app PRIVATE
  .
  ../adapter
  ../c
)

# The cascade is compiled as strict C11 here too, as it is in the C and clang jobs and in the
# FreeRTOS Makefile. It includes no Zephyr header and needs no extension, so nothing
# is given up by holding it to the standard this volume claims for it, and allowing it
# to drift into whatever dialect a build system happens to pass would retire that
# claim one build system at a time.
set_source_files_properties(../c/claim.c PROPERTIES COMPILE_OPTIONS "-std=c11")
