cmake_minimum_required(VERSION 3.22)

project(libs/beeb
        VERSION 0.1
        LANGUAGES CXX)

set(CMAKE_OSX_ARCHITECTURES "arm64")

# AGL.framework was removed from Xcode SDKs (macOS 26+) but still exists in
# the CommandLineTools SDKs. Qt6's FindOpenGL sets WrapOpenGL_AGL before we
# can influence it, so we pre-set the cache variable here (before any
# find_package call) to the correct path inside the CLT SDK.
# FORCE ensures stale cached values from previous configures are overwritten.
if(APPLE)
  file(GLOB _agl_candidates
    "/Library/Developer/CommandLineTools/SDKs/*/System/Library/Frameworks/AGL.framework"
  )
  if(_agl_candidates)
    list(SORT _agl_candidates ORDER DESCENDING)
    list(GET _agl_candidates 0 _agl_path)
    set(WrapOpenGL_AGL "${_agl_path}" CACHE FILEPATH "AGL framework" FORCE)
    message(STATUS "AGL framework: ${WrapOpenGL_AGL}")
  else()
    message(WARNING "AGL.framework not found in CommandLineTools SDKs — OpenGL link may fail")
  endif()
endif()

set(GIT_VERSION_STRING "git version 2.20.1 (Apple Git-117)")
set(CMAKE_CXX_FLAGS_DEBUG "-glldb -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG_COVERAGE "-g -O0 -Wall -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")

# Write targets to bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
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

add_subdirectory(apps)
add_subdirectory(libs)
