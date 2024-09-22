#include "carrier_tone_with_dummy_byte.h"
#include "stream_utils.h"

#include <spdlog/spdlog.h>

CarrierToneWithDummyByte::CarrierToneWithDummyByte(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : Chunk("Carrier") //
{
  pre_byte_cycle_count_ = read_uint_16(uef_stream);
  post_byte_cycle_count_ = read_uint_16(uef_stream);
}

std::string
CarrierToneWithDummyByte::Description() const {
  return fmt::format("Carrier with dummy byte\n"
                     "  pre: {} seconds ({} cycles)\n"
                     "  post: {} seconds ({} cycles)\n",
                     static_cast<float>(pre_byte_cycle_count_) / (DEFAULT_BASE_FREQUENCY * 2.0f),
                     pre_byte_cycle_count_,
                     static_cast<float>(post_byte_cycle_count_) / (DEFAULT_BASE_FREQUENCY * 2.0f),
                     post_byte_cycle_count_);
}
