#include "tape_data_block.h"

#include <sstream>
#include <iomanip>

TapeDataBlock::TapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : TapeDataChunk("Tape Data Block") {
  auto *data = new uint8_t[chunk_length];
  uef_stream->read(reinterpret_cast<char *>(data), chunk_length);

  bytes_.insert(bytes_.end(), data, data + chunk_length);
  delete[] data;
}

std::string
TapeDataBlock::Description() const {
  std::ostringstream oss;

  auto idx = 0;
  oss << "Data" << std::endl << "  ";
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
TapeDataBlock::Data() const {
  return bytes_;
}
