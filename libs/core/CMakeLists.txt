add_library(Core SHARED
        src/clock.cpp
        src/data_connectors.cpp
        src/bus.cpp
)

target_include_directories(Core
        PUBLIC include)

# Clock tests
add_executable(test_clock
        test/main.cpp
        test/test_clock.cpp
)

target_include_directories(test_clock PRIVATE
        test)

target_link_libraries(test_clock PRIVATE
        Core
        gtest
)

# Clock visualiser
add_executable(plot_clock
        src/plot_clock.cpp
)

target_link_libraries(plot_clock PRIVATE
        Core
)
