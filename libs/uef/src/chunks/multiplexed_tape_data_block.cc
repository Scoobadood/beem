#include "multiplexed_tape_data_block.h"

#include <spdlog/spdlog.h>

MultiplexedTapeDataBlock::MultiplexedTapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : TapeDataChunk("Multiplexed Tape Data") {
  spdlog::warn("Multiplexed Tape Data Block not implemented.");
  uef_stream->seekg(chunk_length, std::ios::seekdir::cur);
}

std::string
MultiplexedTapeDataBlock::Description() const {
  return "MPD - Not Implemented";
}

const std::vector<uint8_t> &
MultiplexedTapeDataBlock::Data() const {
  return bytes_;
}
