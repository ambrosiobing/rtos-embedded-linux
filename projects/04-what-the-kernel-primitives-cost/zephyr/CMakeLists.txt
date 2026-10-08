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

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(p04_primitives_zephyr C)

target_sources(app PRIVATE src/main.c ../adapter/measure.c)
