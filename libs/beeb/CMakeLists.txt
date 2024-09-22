add_library(Beeb SHARED
        src/beeb.cpp)

include_directories(Hardware/Common)

target_include_directories(Beeb
        PUBLIC
        include
)

target_link_libraries(Beeb
        PUBLIC
        M6502
        DRAM
        ROM
        Via
        Latch
        SN76489
        Keyboard
        Acia
        2C198_sULA
        Adc
        5C094_vULA
        6845_CRTC
        Common
        Crt
)

# Tests
add_executable(test_beeb
        test/main.cpp
        test/test_beeb_memory.cpp
)

target_include_directories(test_beeb
        PRIVATE test)

target_link_libraries(test_beeb
        PRIVATE
        Beeb
        gtest
)

# Standalone execution
add_executable(RunBeeb
        src/main.cpp
)

target_link_libraries(RunBeeb
        PRIVATE
        Beeb
)