# projects/04-what-the-kernel-primitives-cost/zephyr/CMakeLists.txt
#
# The measurement core is pulled in from ../adapter, not copied. It carries no kernel header, so
# the same object that runs here is the one host/emit_capture.c builds against, and the capture
# format cannot drift between the firmware and the reduction that parses it.
#
#   cmake -B build-p04-zephyr -GNinja -DBOARD=nucleo_h7a3zi_q \
#         -S projects/04-what-the-kernel-primitives-cost/zephyr
#
# A native_sim build is useful for compiling and useless for measuring, and the port says so by
# reporting a clock of zero, which makes the reduction refuse every capture it produces.
cmake_minimum_required(VERSION 3.20.0)

# THE OVERLAY IS NAMED HERE, NOT LEFT TO DETECTION. Zephyr finds an app.overlay in the
# application directory on a fresh configure. On Friday 9 October 2026 the file was added to a
# build directory configured days earlier, ninja re-ran CMake, and the overlay was not found:
# no "Found devicetree overlay" line, and the zephyr,user node the marker reads did not exist.
# A build that silently differs from the tree by which files happened to exist at first
# configure is the stale-flash problem one level up, so the overlay is stated. This variable is
# read by find_package(Zephyr) and must be set before it.
set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/app.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(p04_primitives_zephyr C)

target_sources(app PRIVATE src/main.c src/mutex_case.c ../adapter/measure.c)
