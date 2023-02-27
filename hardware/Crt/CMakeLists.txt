add_library(Crt
        SHARED
        src/crt.cpp)

target_include_directories(Crt
        PUBLIC
        include )

target_link_libraries(Crt
        PUBLIC
        6845_CRTC
        5C094_vULA
        )