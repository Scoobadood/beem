add_library(Cassette SHARED
        src/raw_bytes_tape_stream.cpp
        src/cassette_port.cpp
        src/uef_tape_stream.cpp
        include/i_tape_stream.h
        include/raw_bytes_tape_stream.h
        include/cassette_port.h
        include/uef_tape_stream.h
)

target_include_directories(Cassette
        PUBLIC include)

target_link_libraries(Cassette
        PUBLIC 2C198_sULA UEF)

add_executable(test_cassette
        test/main.cpp
        test/test_raw_bytes_tape_stream.cpp
        test/test_cassette_port.cpp
        test/test_uef_tape_stream.cpp)

target_link_libraries(test_cassette
        PRIVATE
        Cassette
        gtest)
