# projects/03-one-sensor/CMakeLists.txt
#
# One application, two descriptions. The overlay is chosen on the command line rather than
# named here, because naming it here would make this file the thing that differs between the
# two builds, and the chapter's claim is that nothing in the project differs except the
# overlay itself:
#
#   cmake -B build-i2c -GNinja -DBOARD=nucleo_h7a3zi_q #         -DDTC_OVERLAY_FILE=overlays/i2c.overlay -S projects/03-one-sensor
#   cmake -B build-spi -GNinja -DBOARD=nucleo_h7a3zi_q #         -DDTC_OVERLAY_FILE=overlays/spi.overlay -S projects/03-one-sensor
#
# This board needs the Zephyr SDK, which native_sim did not. P01's and P02's adapters build
# with the host compiler and so say nothing about a cross build; this is the first project in
# the volume that cross compiles.
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(one_sensor C)

target_sources(app PRIVATE src/main.c)
