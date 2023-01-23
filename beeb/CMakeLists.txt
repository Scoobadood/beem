add_library(Beeb SHARED
        src/beeb.cpp
        )

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
        Adc
        5C094_vULA
        6845_CRTC
        Common
        )

add_executable(RunBeeb
        src/main.cpp
        )

target_link_libraries(RunBeeb
        PRIVATE
        Beeb
        )
