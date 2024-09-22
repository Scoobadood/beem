#include "explicit_tape_data_block.h"

#include <sstream>
#include <iomanip>

ExplicitTapeDataBlock::ExplicitTapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : TapeDataChunk("Explicit Tape Data Chunk") {

  uint8_t ignored_bit_count;
  uef_stream->get(reinterpret_cast<char&>(ignored_bit_count));
  num_bits_ = chunk_length * 8 - ignored_bit_count;
  auto data_length_bytes = (num_bits_ + 7) % 8;

  auto *data = new uint8_t[data_length_bytes];
  uef_stream->read(reinterpret_cast<char *>(data), chunk_length);

  bytes_.insert(bytes_.end(), data, data + chunk_length);
  delete[] data;
}

std::string
ExplicitTapeDataBlock::Description() const {
  std::ostringstream oss;

  auto idx = 0;
  oss << "Explicit Data ("<<num_bits_<<" bits)" << std::endl << "  ";
  for (auto b : bytes_) {
    oss << "0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t) b << " ";
    idx++;
    if( idx % 8 == 0) {
      oss << std::endl << "  ";
    }
  }
  oss << std::endl;
  return oss.str();
}

const std::vector<uint8_t> &
ExplicitTapeDataBlock::Data() const {
  return bytes_;
}
