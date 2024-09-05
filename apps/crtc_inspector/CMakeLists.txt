add_executable(crtc_inspector
        src/main.cc
        include/crtc_view.h
        src/crtc_view.cc
        src/crtc_view.ui
        src/debuggable_crtc.cc include/debuggable_crtc.h)

target_include_directories(crtc_inspector
        PUBLIC
        include
)

target_link_libraries(crtc_inspector
        PRIVATE
        6845_CRTC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        spdlog::spdlog
)
