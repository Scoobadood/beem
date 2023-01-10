add_executable(Beeb
        src/beeb.cpp
        src/main.cpp
        )

target_include_directories(Beeb PRIVATE
        include
        )

target_link_libraries(Beeb
        PRIVATE
        M6502
        Memory
        Via
        )