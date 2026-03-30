add_library(Via SHARED
        src/6522_via.cpp
        )

target_include_directories(Via
        PUBLIC
        include
        )

target_link_libraries(Via
        PUBLIC
        Bus
        Common
        )

# ---------------------------------------------------------------------------
# vrEmu6522 reference oracle (MIT, Troy Schrapel)
# Use FetchContent_Populate (not MakeAvailable) to pull the source only,
# avoiding inheritance of vrEmu6522's own -Werror / -march=native flags.
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
        vrEmu6522
        GIT_REPOSITORY https://github.com/visrealm/vrEmu6522.git
        GIT_TAG        main
)
FetchContent_GetProperties(vrEmu6522)
if(NOT vremu6522_POPULATED)
    FetchContent_Populate(vrEmu6522)
endif()

add_executable(test_via
        test/test_via.cpp
        test/test_via_oracle.cpp
        test/main.cpp
        ${vremu6522_SOURCE_DIR}/src/vrEmu6522.c
        )

# Compile the C oracle source as C++ (project language is CXX only)
set_source_files_properties(${vremu6522_SOURCE_DIR}/src/vrEmu6522.c
        PROPERTIES LANGUAGE CXX)

target_compile_definitions(test_via PRIVATE VR_6522_EMU_STATIC)

target_include_directories(test_via
        PRIVATE
        test
        ${vremu6522_SOURCE_DIR}/src
        )

target_link_libraries(test_via
        PRIVATE
        gtest
        Via
        )

add_test(NAME ViaTests COMMAND test_via)

