add_library(6845_CRTC SHARED
        src/6845_crtc.cpp
        )

target_include_directories(6845_CRTC PUBLIC
        include)

target_link_libraries(6845_CRTC PUBLIC
        Bus
        Common)



add_executable(test_crtc
        test/test_crtc.cpp
)


target_link_libraries(test_crtc
        PRIVATE
        gtest
        6845_CRTC
)