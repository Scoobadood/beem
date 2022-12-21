cmake_minimum_required(VERSION 3.22)
project(m6502
        VERSION 0.1
        LANGUAGES CXX)

set(CMAKE_OSX_ARCHITECTURES "arm64")

set(GIT_VERSION_STRING "git version 2.20.1 (Apple Git-117)")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG_COVERAGE "-g -O0 -Wall -fprofile-arcs -ftest-coverage")

# Write targets to bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 11)

add_executable(cpu
        src/main.cpp
        include/cpu.h src/cpu.cpp
        include/addressing.h src/addressing.cpp
        include/opcodes.h src/opcodes.cpp)

target_include_directories(cpu
        PRIVATE
        include)
