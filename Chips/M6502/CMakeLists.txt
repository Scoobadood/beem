add_library(M6502 SHARED
        include/m6502.h src/m6502.cpp
        include/memory.h src/memory.cpp
        include/cycle_handler.h src/cycle_handler.cpp
        include/opcodes.h src/opcodes.cpp
        )

target_include_directories(M6502 PUBLIC
        include
        )

target_link_libraries(M6502
        PUBLIC Bus)

add_subdirectory(tests)