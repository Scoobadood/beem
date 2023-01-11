add_library(SN76489 SHARED
        src/sound_76489.cpp
        )

target_include_directories(SN76489
        PUBLIC
        include
        )

target_link_libraries(SN76489
        PUBLIC
        Via
        )