#include "custom_tape_data_block.h"

#include <spdlog/spdlog.h>

#include <sstream>
#include <iomanip>

CustomTapeDataBlock::CustomTapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length)
    : TapeDataChunk("Custom Tape Data") {
  uint8_t meta_data[3];
  uef_stream->read(reinterpret_cast<char *>(meta_data), 3);

  bits_per_packet_ = meta_data[0];
  switch (meta_data[1]) {
    case 'N':
      parity_ = NONE;
      break;
    case 'E':
      parity_ = EVEN;
      break;
    case 'O':
      parity_ = ODD;
      break;
    default:
      throw std::runtime_error(fmt::format("Invalid parity {:02x} in CustomTapeDataBlock"));
  }
  auto sbc = (int8_t) meta_data[2];
  if (sbc < 0) {
    stop_bits_ = -sbc;
    include_short_wave_ = true;
  }
  else {
    stop_bits_ = sbc;
    include_short_wave_ = false;
  }
  auto byte_data = new uint8_t[chunk_length - 3];
  uef_stream->read(reinterpret_cast<char *>(byte_data), chunk_length - 3);
  bytes_.insert(bytes_.end(), byte_data, byte_data + chunk_length - 3);
}

std::string
CustomTapeDataBlock::Description() const {
  std::ostringstream oss;

  oss << "Custom Tape Data" << std::endl;
  oss << "  bits     : " << bits_per_packet_ << std::endl;
  oss << "  stop bits: " << stop_bits_ << std::endl;
  oss << "  parity   : " << ((parity_ == NONE) ? "NONE" : ((parity_ == ODD) ? "ODD" : "EVEN")) << std::endl;
  oss << "  ";

  auto idx = 0;
  for (auto b : bytes_) {
    oss << "0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t) b << " ";
    idx++;
    if (idx % 8 == 0) {
      oss << std::endl << "  ";
    }
  }
  oss << std::endl;
  return oss.str();
}

const std::vector<uint8_t> &
CustomTapeDataBlock::Data() const {
  return bytes_;
}
