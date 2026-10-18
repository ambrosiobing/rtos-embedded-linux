# projects/05-the-zones/CMakeLists.txt
#
# One application, and the overlay is chosen on the command line rather than named here, which
# is P03's arrangement and its reason: a file that names the overlay becomes the thing that
# differs between two builds, and both projects want the description to be the only difference.
#
# THE CHEAP ATTEMPT FIRST. Before a driver is written for this part, the part is pointed at the
# driver the tree already carries for the generation before it, and what happens on the wire is
# reported either way. That is the step a reader asks about first and it costs one build:
#
#   cmake -B build-p05-existing -GNinja -DBOARD=nucleo_h7a3zi_q \
#         -DDTC_OVERLAY_FILE=overlays/existing_driver.overlay -S projects/05-the-zones
#   ninja -C build-p05-existing
#
# THE ENVIRONMENT COMES FIRST, and it is written here because its absence does not look like
# itself. West lives in the workspace's virtual environment on the machine this is built on:
#
#   source ~/zephyrproject/.venv/bin/activate
#
# Without it, a FRESH configure finds no modules, the vendor's devicetree directory is off the
# include path, and the board's own description fails to preprocess on a pinctrl include that is
# on disk the whole time. Established on Saturday 17 October 2026 by four configures, of which
# only the fourth was a test: the one that appeared to clear the environment had reused the build
# directory from the failure and so read a cached configure. **Remove the build directory in the
# same command as the test**, or the test is of the cache.
#
# THE SECOND DESCRIPTION, which points the part at this project's own driver and binding, is
# built the same way with a different overlay and its own build directory:
#
#   cmake -B build-p05-own -GNinja -DBOARD=nucleo_h7a3zi_q #         -DDTC_OVERLAY_FILE=overlays/own_driver.overlay -S projects/05-the-zones
#   ninja -C build-p05-own
#
# THE FIRMWARE IMAGE IS NOT IN THE TREE and the part cannot range without it. The vendor's
# library ships with the buffer header removed, because the operating system's rules do not
# admit a binary blob in its source, so the image is fetched once into the workspace:
#
#   west blobs fetch hal_st
#
# A run that skips it fails at initialisation, which is easy to read as a wiring fault or a
# wrong address. docs/DESIGN.md says so in more words.
cmake_minimum_required(VERSION 3.20.0)

# A LINE THAT WAS HERE AND IS NOT, AND THE REMOVAL IS THE RECORD. It read
#
#   list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
#
# and it was added on Saturday 17 October 2026 to state where this project's own bindings live
# rather than leave it to detection, which is P04's lesson about a stated overlay carried one
# level across. It is removed because the operating system already treats an application's own
# directory as a place to look for bindings, so the line said nothing the build did not know, and
# because it was the only change in this file that had never been through a successful configure
# when one began to fail. Whether it was the cause is settled by the build rather than here.
#
# The lesson it was carrying is still right and now has nowhere better to live than this comment:
# the bindings this project writes are under dts/bindings/, and a reader looking for them should
# not have to know the default to find them.

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(the_zones C)

target_sources(app PRIVATE src/main.c src/shield_power.c src/identify.c src/vl53l8cx.c)

# ==================================================== the vendor's interface for this generation
#
# NOTHING IS VENDORED, which is this volume's rule and the operating system's as well. The
# vendor's library is cloned at a pinned commit and compiled from where it lies; no copy of it,
# and no copy of the firmware image it carries, enters this repository. P04 brings in a kernel
# and a board support package the same way and for the same reasons: tens of thousands of lines
# nobody here wrote stay out of a published tree, and the version becomes part of the build
# rather than part of the history.
#
#   git clone https://github.com/STMicroelectronics/x-cube-tof1.git ~/src/x-cube-tof1
#   git -C ~/src/x-cube-tof1 checkout c12709c3ac802d91dd2a215178f53cb94c024709
#
# THE PIN IS WHAT WAS BUILT AGAINST and not what was intended, which is P04's lesson about a
# pin arriving the hard way. The clone on the demo laptop landed on c12709c, dated Thursday 3
# April 2025, and every figure this project takes comes from that revision until the line is
# changed deliberately and the change is dated.
#
# AND UNTIL SUNDAY 18 OCTOBER 2026 THAT PARAGRAPH WAS NOT TRUE, which is worth keeping rather
# than quietly fixing. The revision appeared in this file only inside the comment above: no cmake
# command read it, so the build compiled whatever revision happened to be checked out in the
# clone, and a `git pull` in that directory would have moved every figure this project publishes
# without changing a line here. The claim that "the version becomes part of the build rather than
# part of the history" was a claim about a comment. **A rule named in prose and enforced by
# nothing is this volume's most repeated defect**, and the fix is the eight lines below.
#
# Override VL53L8CX_ULD to point somewhere else.
set(VL53L8CX_ULD "$ENV{HOME}/src/x-cube-tof1/Drivers/BSP/Components/vl53l8cx/modules"
    CACHE PATH "the vendor's ultra lite driver for this generation, cloned not copied")

set(VL53L8CX_ULD_COMMIT "c12709c3ac802d91dd2a215178f53cb94c024709"
    CACHE STRING "the vendor revision every figure in this project was measured against")

# THE ABSENCE IS REPORTED WITH THE COMMAND THAT FIXES IT, because a missing clone otherwise
# arrives as a compiler saying it cannot find a header, which names the symptom and not the step.
if(NOT EXISTS "${VL53L8CX_ULD}/vl53l8cx_api.c")
  message(FATAL_ERROR
    "No vendor interface at ${VL53L8CX_ULD}.
"
    "  git clone https://github.com/STMicroelectronics/x-cube-tof1.git ~/src/x-cube-tof1
"
    "Nothing is vendored into this repository, so it has to be cloned once.")
endif()

# THE CLONE IS ASKED WHAT REVISION IT IS AT, because the alternative is to trust it. `git -C` on
# the modules subdirectory walks up to the clone's own root, so no second path is needed.
execute_process(
  COMMAND git -C "${VL53L8CX_ULD}" rev-parse HEAD
  OUTPUT_VARIABLE uld_head
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE uld_rc
  ERROR_QUIET)

if(NOT uld_rc EQUAL 0)
  message(FATAL_ERROR
    "Cannot ask git what revision ${VL53L8CX_ULD} is at.
"
    "That directory exists but is not inside a git clone, which usually means the vendor's source
"
    "was copied rather than cloned. This project states the revision every figure came from, so
"
    "it has to be able to read it:
"
    "  git clone https://github.com/STMicroelectronics/x-cube-tof1.git ~/src/x-cube-tof1
"
    "  git -C ~/src/x-cube-tof1 checkout ${VL53L8CX_ULD_COMMIT}")
elseif(NOT uld_head STREQUAL "${VL53L8CX_ULD_COMMIT}")
  # DELIBERATELY FATAL. Building against another revision is a legitimate thing to want and it is
  # not something to do by accident: every figure in docs/RESULTS and every number in the chapter
  # is attributed to the pinned one. The escape hatch is explicit and says so in the log.
  if(VL53L8CX_ULD_ANY_REVISION)
    message(WARNING
      "The vendor clone is at ${uld_head}, not the pinned ${VL53L8CX_ULD_COMMIT}, and "
      "VL53L8CX_ULD_ANY_REVISION is set, so this build is going ahead. Nothing measured from it "
      "may be quoted beside a figure taken against the pin without saying so.")
  else()
    message(FATAL_ERROR
      "The vendor clone is at a different revision than this project is written against.
"
      "  clone is at   ${uld_head}
"
      "  pinned here   ${VL53L8CX_ULD_COMMIT}
"
      "Every figure this project publishes was measured against the pinned one, so the build
"
      "refuses rather than quietly producing numbers that belong to another revision:
"
      "  git -C ${VL53L8CX_ULD} checkout ${VL53L8CX_ULD_COMMIT}
"
      "To build against the clone's revision on purpose, configure with
"
      "-DVL53L8CX_ULD_ANY_REVISION=ON, which warns instead and marks the build as unpinned.")
  endif()
else()
  message(STATUS "vendor interface at the pinned ${VL53L8CX_ULD_COMMIT}")
endif()

# The porting layer is ours and comes first on the include path, so the library finds this
# project's platform.h rather than the vendor's reference one. src/platform.h says why.
target_include_directories(app PRIVATE src "${VL53L8CX_ULD}")

target_sources(app PRIVATE
  src/vl53l8cx_platform.c
  "${VL53L8CX_ULD}/vl53l8cx_api.c")

# THE LIBRARY IS NOT HELD TO THIS PROJECT'S WARNINGS. It is somebody else's code, used
# unmodified, and a warning in it is not a defect this repository can fix without forking it.
set_source_files_properties("${VL53L8CX_ULD}/vl53l8cx_api.c" PROPERTIES COMPILE_OPTIONS "-w")

# ================================================= what the image is, regenerated at every build
#
# THIS IS A CORRECTION AND NOT A FEATURE. The banner used to print `__DATE__` and `__TIME__` from
# main.c. Those two macros are fixed when that one file is compiled, and a build that changes any
# other file does not recompile it: on Saturday 17 October 2026 at 16:51 an image built from an
# edit to the driver printed a stamp 69 minutes old. The reading that followed was nearly thrown
# away as a stale flash, which is the dangerous direction for a guard to fail in.
#
# THE COMMAND RUNS EVERY BUILD, BY CONSTRUCTION. A custom target with no output is always out of
# date, so a command that depends on one is never skipped. The generated file therefore changes
# on every build, its translation unit recompiles, and the image is relinked. That cost is one
# small compile and one link, paid so the console cannot claim an age the image does not have.
# `ninja` in this project is consequently never a no-op, which is the intended arrangement.
#
# THE OUTPUT FORM IS USED RATHER THAN A BYPRODUCT, so the build system has a real rule producing
# a real file and the first build cannot race its own generator.
set(BUILD_STAMP_C "${CMAKE_CURRENT_BINARY_DIR}/build_stamp.c")
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# THE INTERPRETER IS THE ONE THE OPERATING SYSTEM'S BUILD ALREADY FOUND, because the build runs
# inside a virtual environment where the bare name resolves differently than outside it. The
# fallback exists so that a missing variable arrives as a sentence about Python rather than as a
# command that cannot be run.
if(NOT DEFINED PYTHON_EXECUTABLE)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
endif()

add_custom_target(stamp_always)

add_custom_command(
  OUTPUT "${BUILD_STAMP_C}"
  COMMAND "${PYTHON_EXECUTABLE}" "${REPO_ROOT}/scripts/stamp.py"
          --out "${BUILD_STAMP_C}"
          --scope "projects/05-the-zones"
  DEPENDS stamp_always "${REPO_ROOT}/scripts/stamp.py"
  COMMENT "stamping the image with the time and the commit"
  VERBATIM)

target_sources(app PRIVATE "${BUILD_STAMP_C}")
