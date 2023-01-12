add_library(Common SHARED
        src/data_connectors.cpp)

target_include_directories(Common
        PUBLIC include)