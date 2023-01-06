add_executable(test-M6502
        test-6502.cpp
        )

target_link_libraries(test-M6502
        PRIVATE
        M6502
        )