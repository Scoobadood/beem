add_library(Disassembler
        SHARED
        src/disassembler.cc include/Disassembler/disassembler.h
        src/symbol_file_loader.cc include/Disassembler/symbol_file_loader.h
        src/operation_formatter.cc include/Disassembler/operation_formatter.h
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
