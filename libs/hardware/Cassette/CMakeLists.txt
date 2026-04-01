add_library(Cassette SHARED
        src/raw_bytes_tape_stream.cpp
        src/cassette_port.cpp
        include/i_tape_stream.h
        include/raw_bytes_tape_stream.h
        include/cassette_port.h
)

target_include_directories(Cassette
        PUBLIC include)

target_link_libraries(Cassette
        PUBLIC 2C198_sULA)

add_executable(test_cassette
        test/main.cpp
        test/test_raw_bytes_tape_stream.cpp
        test/test_cassette_port.cpp)

target_link_libraries(test_cassette
        PRIVATE
        Cassette
        gtest)
