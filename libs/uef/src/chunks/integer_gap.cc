#include "integer_gap.h"
#include "stream_utils.h"

#include <spdlog/spdlog.h>

IntegerGap::IntegerGap(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
: Chunk( "Integer Gap")
{
  duration_ = read_uint_16(uef_stream);
}

std::string
IntegerGap::Description() const {
  return fmt::format("Integer Gap\n  Silent {} seconds ({} cycles)\n",
                     duration_ / (2.0 * DEFAULT_BASE_FREQUENCY),
                     duration_);
}
