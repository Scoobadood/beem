# ------------------------------------------------------------------------------
# zlib
FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
)
FetchContent_MakeAvailable(zlib)


add_library(UEF
        SHARED
        src/uef.cc include/UEF/uef.h
        src/stream_utils.cc include/UEF/stream_utils.h
        src/uef_tape_loader.cc

        src/chunks/chunk.cc include/UEF/chunk.h
        src/chunks/origin_chunk.cc include/UEF/origin_chunk.h
        src/chunks/unknown_chunk.cc include/UEF/unknown_chunk.h
        src/chunks/carrier_tone.cc include/UEF/carrier_tone.h
        src/chunks/tape_data_block.cc include/UEF/tape_data_block.h
        src/chunks/explicit_tape_data_block.cc include/UEF/explicit_tape_data_block.h
        src/chunks/integer_gap.cc include/UEF/integer_gap.h
        src/chunks/floating_point_gap.cc include/UEF/floating_point_gap.h
        src/chunks/multiplexed_tape_data_block.cc include/UEF/multiplexed_tape_data_block.h
        src/chunks/custom_tape_data_block.cc include/UEF/custom_tape_data_block.h
        src/chunks/carrier_tone_with_dummy_byte.cc include/UEF/carrier_tone_with_dummy_byte.h
        src/chunks/phase_change.cc include/UEF/phase_change.h
        src/chunks/frequency_change.cc include/UEF/frequency_change.h
        src/chunks/security_cycles.cc include/UEF/security_cycles.h

        include/UEF/tape_data_chunk.h
        src/acorn_block.cc include/UEF/acorn_block.h
        src/crc-16-acorn.cc include/UEF/crc-16-acorn.h
)

target_include_directories(UEF
        PUBLIC include
        PRIVATE include/UEF
)

target_link_libraries(UEF
        PRIVATE
        zlib
)