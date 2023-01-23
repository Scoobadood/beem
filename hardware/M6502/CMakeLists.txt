add_library(M6502 SHARED
        src/m6502.cpp
        src/cycle_handler.cpp
        src/opcodes.cpp
        )

target_include_directories(M6502 PUBLIC
        include
        )

target_link_libraries(M6502
        PUBLIC
        Bus
        DRAM)

add_subdirectory(tests)