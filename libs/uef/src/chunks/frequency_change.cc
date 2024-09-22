#include "frequency_change.h"

#include <spdlog/spdlog.h>

#include "stream_utils.h"

FrequencyChange::FrequencyChange(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
: Chunk( "Frequency Change") {
  new_frequency_ = read_iee_754_float(uef_stream);
}

std::string
FrequencyChange::Description() const {
  return fmt::format("Frequency Change\n  New frequency {}\n", new_frequency_);
}
