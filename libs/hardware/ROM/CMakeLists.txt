add_library(ROM SHARED
        src/rom.cpp
        )

target_include_directories(ROM PUBLIC
        include
        )

target_link_libraries(ROM PUBLIC
        Bus
        )