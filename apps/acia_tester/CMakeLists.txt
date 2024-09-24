add_executable(acia_tester
        src/main.cc
        src/acia_test_window.cc src/acia_test_window.ui include/acia_test_window.h
        include/debuggable_acia.h
)

target_include_directories(acia_tester
        PRIVATE include
)

target_link_libraries(acia_tester
        PRIVATE
        Acia
        2C198_sULA
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        spdlog::spdlog

)