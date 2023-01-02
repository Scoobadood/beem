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

# Spdlog
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.x
)
FetchContent_MakeAvailable(spdlog)
include_directories(${spdlog_SOURCE_DIR}/include)

add_executable(test-6502
        src/test-6502.cpp
        include/cpu.h src/cpu.cpp
        include/addressing.h src/addressing.cpp
        include/opcodes.h src/opcodes.cpp
        include/memory.h src/memory.cpp
        include/via.h src/via.cpp
        include/sound_76489.h src/sound_76489.cpp
        include/keyboard.h src/keyboard.cpp

        )

target_include_directories(test-6502
        PRIVATE
        include
        )


add_executable(beeb
        src/main.cpp
        include/cpu.h src/cpu.cpp
        include/addressing.h src/addressing.cpp
        include/opcodes.h src/opcodes.cpp
        include/memory.h src/memory.cpp
        include/display.h src/display.cpp
        include/via.h src/via.cpp
        include/beeb.h src/beeb.cpp
        include/keyboard.h src/keyboard.cpp
        include/sound_76489.h src/sound_76489.cpp
        )


target_include_directories(beeb
        PRIVATE
        include)
