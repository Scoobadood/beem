add_library(6845_CRTC SHARED
        src/6845_crtc.cpp)

target_include_directories(6845_CRTC PUBLIC
        include)

target_link_libraries(6845_CRTC PUBLIC
        Bus)

add_executable(test_CRTC
        test/main.cpp
        test/test_mode_0.cpp)

target_include_directories(test_CRTC PRIVATE test)

target_link_libraries(test_CRTC PRIVATE
        gtest
        6845_CRTC)
