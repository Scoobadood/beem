# Qt6 for corr_tool and animesh
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


list(APPEND CMAKE_PREFIX_PATH "/Users/dave/Qt/6.7.0/macos/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)


add_subdirectory(acia_tester)
add_subdirectory(beeb)
add_subdirectory(crtc_inspector)
add_subdirectory(uef_tools)
