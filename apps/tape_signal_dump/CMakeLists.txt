add_executable(tape-signal-dump
        src/main.cc
)

target_link_libraries(tape-signal-dump
        PRIVATE Cassette
)
