cmake_minimum_required(VERSION 3.22)
project(Chips/M6502
        VERSION 0.1
        LANGUAGES CXX)

set(CMAKE_OSX_ARCHITECTURES "arm64")

set(GIT_VERSION_STRING "git version 2.20.1 (Apple Git-117)")
set(CMAKE_CXX_FLAGS_DEBUG "-glldb -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG_COVERAGE "-g -O0 -Wall -fprofile-arcs -ftest-coverage")

# Write targets to bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



include(FetchContent)

# ------------------------------------------------------------------------------
# A Fast C++ logging library.

# Spdlog
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.x
)
FetchContent_MakeAvailable(spdlog)
include_directories(${spdlog_SOURCE_DIR}/include)

# ------------------------------------------------------------------------------
# GogleTest
# Include Google test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable Test
enable_testing()

add_subdirectory(hardware)
add_subdirectory(beeb)
add_subdirectory(debugger)
add_subdirectory(roms)
add_subdirectory(common)
add_subdirectory(video_subsystem_test)