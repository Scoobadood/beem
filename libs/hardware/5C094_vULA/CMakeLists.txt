add_library(5C094_vULA SHARED
        src/5c094_vula.cpp)

target_include_directories(5C094_vULA
        PUBLIC
        include)

target_link_libraries(5C094_vULA
        PUBLIC
        Core
        6845_CRTC)


# Tests
add_executable(test_vula
        test/main.cpp
        test/test_ula.cpp
        )

target_include_directories(test_vula PRIVATE test)

target_link_libraries(test_vula PUBLIC
        gtest
        5C094_vULA)

