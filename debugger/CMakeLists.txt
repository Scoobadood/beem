# Qt6 for corr_tool and animesh
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


list(APPEND CMAKE_PREFIX_PATH "/Users/dave/Qt_5.15/6.2.4/macos/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(deeb
        src/main.cpp
        src/disassembler.cpp
        src/disassembly_view.cpp
        src/reg_view.cpp src/reg_view.ui include/reg_view.h
        src/bus_view.cpp src/bus_view.ui include/bus_view.h
        src/deeb_window.cpp src/deeb_window.ui include/deeb_window.h
        src/memory_view.cpp src/memory_view.ui include/memory_view.h
        src/breakpoint_dlg.cpp src/breakpoint_dlg.ui include/breakpoint_dlg.h
        )

target_include_directories(deeb
        PUBLIC include)

target_link_libraries(deeb
        PRIVATE
        Beeb
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        )


# Testing
add_executable(test_disassembler
        tests/test_disassembler.h tests/test_disassmbler.cpp tests/main.cpp
        src/disassembler.cpp include/disassembler.h
        )

target_include_directories(test_disassembler
        PRIVATE include)

target_link_libraries(test_disassembler
        PRIVATE
        Beeb
        gtest
        gmock
        )
