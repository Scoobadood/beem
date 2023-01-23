add_library(DRAM SHARED
        src/dram.cpp
        )

target_include_directories(DRAM PUBLIC
        include
        )

target_link_libraries(DRAM
        PUBLIC Bus
        )
