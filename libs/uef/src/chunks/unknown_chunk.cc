#include "unknown_chunk.h"

#include <spdlog/spdlog.h>

UnknownChunk::UnknownChunk(uint16_t chunk_id)
    : Chunk{fmt::format("Unknown {:04x}", chunk_id)} //
{
  spdlog::warn("Found unrecognised chunk {:04x}", chunk_id);
}

std::string
UnknownChunk::Description() const {
  return "...\n";
}
