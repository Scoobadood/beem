add_library(Acia SHARED
        src/acia_6850.cpp)

target_include_directories(Acia
        PUBLIC
        include
        )

target_link_libraries(Acia
        PUBLIC
        Core
        )

add_executable(test_acia
        test/main.cpp
        test/test_acia.cpp)

target_include_directories(test_acia
        PRIVATE test)

target_link_libraries(test_acia
        PRIVATE
        Acia
        gtest)