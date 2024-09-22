#include "security_cycles.h"

#include <spdlog/spdlog.h>

SecurityCycles::SecurityCycles(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
: Chunk( "Security Cycles"){
  spdlog::warn("Security Cycles not implemented.");
  uef_stream->seekg(chunk_length, std::ios::seekdir::cur);
}

std::string
SecurityCycles::Description() const {
  return "Security Cycles - Not implemented\n";
}
