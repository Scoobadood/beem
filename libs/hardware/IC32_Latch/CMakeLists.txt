add_library(Latch SHARED
        IC32Latch.cpp)

target_include_directories(Latch
        PUBLIC .)

target_link_libraries(Latch
        PUBLIC
        Via)