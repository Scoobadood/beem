#include "carrier_tone.h"
#include "stream_utils.h"

#include <spdlog/spdlog.h>

CarrierTone::CarrierTone(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : Chunk("Carrier") //
{
  cycle_count_ = read_uint_16(uef_stream);
}

std::string
CarrierTone::Description() const {
  return fmt::format("Carrier\n  {} seconds ({} cycles)\n",
                     static_cast<float>(cycle_count_) / (DEFAULT_BASE_FREQUENCY * 2.0f),
                     cycle_count_);
}
