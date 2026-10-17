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
# Without it, `west` is not on the path and anything that needs west fails, which on Saturday 17
# October 2026 was `west blobs fetch`. It was ALSO blamed that day for a configure failure it had
# nothing to do with: the configure was run again with the environment activated and failed in
# exactly the same way. The line stays because activating the environment is right; the story
# attached to it was wrong and is not repeated here.
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
