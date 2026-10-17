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
# THE FIRMWARE IMAGE IS NOT IN THE TREE and the part cannot range without it. The vendor's
# library ships with the buffer header removed, because the operating system's rules do not
# admit a binary blob in its source, so the image is fetched once into the workspace:
#
#   west blobs fetch hal_st
#
# A run that skips it fails at initialisation, which is easy to read as a wiring fault or a
# wrong address. docs/DESIGN.md says so in more words.
cmake_minimum_required(VERSION 3.20.0)

# THE BINDING DIRECTORY IS STATED RATHER THAN LEFT TO DETECTION, which is P04's lesson one level
# across. That project added an overlay to a build directory configured days earlier, the file
# was never found, and the build silently differed from the tree by which files happened to exist
# at first configure. The operating system does look for bindings under an application by
# default; saying so here costs one line and means a reader can see where dts/bindings/ is picked
# up instead of having to know. This variable is read by find_package and must be set before it.
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(the_zones C)

target_sources(app PRIVATE src/main.c src/shield_power.c src/identify.c)
