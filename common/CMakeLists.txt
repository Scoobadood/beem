add_library(Common SHARED
        src/data_connectors.cpp
        src/clock.cpp)

target_include_directories(Common
        PUBLIC include)


# Tests
add_executable(test_clock
        test/main.cpp
        test/test_clock.cpp
        )

target_include_directories(test_clock PRIVATE
        include)

target_link_libraries(test_clock PRIVATE
        Common
        gtest
        )

# Plotter
add_executable(plot_clock
        src/plot_clock.cpp
        )

target_link_libraries(plot_clock PRIVATE
        Common
        )


