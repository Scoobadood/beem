add_library(Disassembler
        SHARED
        src/disassembler.cc include/Disassembler/disassembler.h
)

target_include_directories(Disassembler
        PRIVATE include/Disassembler
        PUBLIC include
)

target_link_libraries(Disassembler
        PRIVATE M6502
)


# Testing
add_executable(test_disassembler
        tests/test_disassembler.h tests/test_disassembler.cc tests/main.cc
)

target_include_directories(test_disassembler
        PRIVATE tests
)

target_link_libraries(test_disassembler
        PRIVATE
        Beeb
        Disassembler
        gtest
        gmock
)
