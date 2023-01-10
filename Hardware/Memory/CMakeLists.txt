add_library(Memory SHARED
        src/memory.cpp
        )

target_include_directories(Memory PUBLIC
        include
        )

target_link_libraries(Memory
        PUBLIC Bus
        )
