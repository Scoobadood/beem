add_library(Adc SHARED
        src/adc.cpp)

target_include_directories(Adc
        PUBLIC include)

target_link_libraries(Adc
        PUBLIC
        Bus
        )
