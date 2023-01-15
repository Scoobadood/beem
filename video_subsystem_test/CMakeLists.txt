add_executable(test_video
        test_video.cpp)

target_include_directories(test_video PRIVATE .)

target_link_libraries(test_video PRIVATE
        Bus
        5C095_vULA
        6845_CRTC)