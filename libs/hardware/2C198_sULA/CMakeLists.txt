add_library(2C198_sULA SHARED
        src/2c198_sula.cc include/2c198_sula.h
)

target_include_directories(2C198_sULA
        PUBLIC  include)

target_link_libraries(2C198_sULA
        PUBLIC
        Core
        Acia
)

add_executable(test_sula
        test/main.cpp
        test/test_sula.cpp)

target_include_directories(test_sula
        PRIVATE test)

target_link_libraries(test_sula
        PRIVATE
        2C198_sULA
        gtest)