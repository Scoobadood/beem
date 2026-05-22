add_library(Keyboard SHARED
        src/keyboard.cpp)

target_include_directories(Keyboard
        PUBLIC
        include)

target_link_libraries(Keyboard
        PUBLIC Core
)

add_executable(testKeyboard
        test/main.cpp
        test/TestKeyboardLEDs.cpp
        test/TestKeyboardAutoScan.cpp

        )

target_include_directories(testKeyboard
        PRIVATE
        test)

target_link_libraries(testKeyboard
        PUBLIC
        Keyboard
        gtest
        gmock)

add_test(
        NAME CapsLockTurnsOn
        COMMAND testKeyboardLEDs --gtest_filter=CapsLockTurnsOn
)
