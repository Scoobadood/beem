#include "floating_point_gap.h"

#include <spdlog/spdlog.h>

#include "stream_utils.h"

FloatingPointGap::FloatingPointGap(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : Chunk{"Floating Point Gap"} {
  duration_ = read_iee_754_float(uef_stream);
}

std::string
FloatingPointGap::Description() const {
  return fmt::format("Floating Point Gap\n  Silent {} seconds\n",duration_);
}
