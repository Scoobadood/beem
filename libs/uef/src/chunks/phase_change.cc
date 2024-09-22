#include "phase_change.h"

#include <spdlog/spdlog.h>
#include "stream_utils.h"

PhaseChange::PhaseChange(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : Chunk("Phase Change") {
  new_phase_ = read_uint_16(uef_stream);
}

std::string
PhaseChange::Description() const {
  return fmt::format("Phase Change\n  New phase {}\n", new_phase_);
}