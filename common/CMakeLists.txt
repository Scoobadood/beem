add_library(Common SHARED
        src/data_connectors.cpp
        src/clock.cpp
        )

target_include_directories(Common
        PUBLIC include)


add_executable(plot_clock
        src/plot_clock.cpp
        )

target_link_libraries(plot_clock PRIVATE
        Common
        )


