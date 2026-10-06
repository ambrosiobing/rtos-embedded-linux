# projects/03-one-sensor/diag/CMakeLists.txt
#
# A SEPARATE APPLICATION, deliberately. The project's own application must not change: its
# object is the evidence for criterion 1, and both builds of it have already been compared.
# A diagnostic that edited src/main.c would invalidate that comparison to answer a question
# about a breadboard.
#
# This one is allowed to know about the bus, because finding out about the bus is its whole
# purpose. It includes i2c.h, which the application next door is not permitted to.
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(one_sensor_diag C)

target_sources(app PRIVATE src/main.c)
