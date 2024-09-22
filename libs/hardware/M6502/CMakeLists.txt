add_library(M6502 SHARED
        src/m6502.cpp include/6502/m6502.h
        src/cycle_handler.cpp include/6502/cycle_handler.h
        src/opcodes.cpp include/6502/opcodes.h
)

target_include_directories(M6502
        PRIVATE include/6502
        PUBLIC include
)

target_link_libraries(M6502
        PUBLIC
        Bus
        DRAM)

add_subdirectory(tests)