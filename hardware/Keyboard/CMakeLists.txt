add_library(Keyboard SHARED
        src/keyboard.cpp)

target_include_directories(Keyboard
        PUBLIC
        include)

target_link_libraries(Keyboard
        PUBLIC Common
)

