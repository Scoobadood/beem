add_library(Via SHARED
        src/6522_via.cpp
        )

target_include_directories(Via
        PUBLIC
        include
        )

target_link_libraries(Via
        PUBLIC
        Bus
        )

