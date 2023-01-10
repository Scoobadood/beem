add_library(Bus SHARED
        src/bus.cpp
        )

target_include_directories(Bus
        PUBLIC include)

#add_subdirectory(tests)