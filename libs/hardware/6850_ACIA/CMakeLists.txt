add_library(Acia SHARED
        src/acia_6850.cpp)

target_include_directories(Acia
        PUBLIC
        include
        )

target_link_libraries(Acia
        PUBLIC
        Bus
        Common
        )