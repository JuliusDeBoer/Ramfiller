cmake_minimum_required(VERSION 3.14)

project(ramfiller VERSION 1.0)

add_definitions(-DPROJ_VERSION="${PROJECT_VERSION}")

add_executable(ramfiller src/ramfiller.c)

set_property(TARGET ramfiller PROPERTY C_STANDARD 99)
