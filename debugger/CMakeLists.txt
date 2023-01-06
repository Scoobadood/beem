# Qt6 for corr_tool and animesh
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_PREFIX_PATH "/Users/dave/Qt_5.15/6.2.4/macos/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(deeb
        src/deeb_window.cpp  include/deeb_window.h src/deeb_window.ui
        src/disasm_view.cpp include/disasm_view.h src/disasm_view.ui
        src/main.cpp
        src/disassembler.cpp include/disassembler.h
        )

target_include_directories(deeb
        PUBLIC include)

target_link_libraries(deeb
        PRIVATE
        M6502
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
        M6502
        gtest
        gmock
        )
