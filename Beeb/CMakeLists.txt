add_library(Beeb SHARED
        src/beeb.cpp
        )

target_include_directories(Beeb
        PUBLIC
        include
        )

target_link_libraries(Beeb
        PUBLIC
        M6502
        Memory
        Via
        Latch
        )

add_executable(RunBeeb
        src/main.cpp
        )

target_link_libraries(RunBeeb
        PRIVATE
        Beeb
        )
