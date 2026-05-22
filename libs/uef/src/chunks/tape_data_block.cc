#include "tape_data_block.h"

#include <sstream>
#include <iomanip>

#include "../../../../../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1/strstream"

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

  std::ostringstream hex;
  std::ostringstream asc;

  for (auto b : bytes_) {
    hex << std::hex << std::setw(2) << std::setfill('0') << (uint32_t) b << " ";
    asc << (isprint(b) ? ((char) b) : '.');
    idx++;
    if( idx % 8 == 0) {
      oss << hex.str() << " | " << asc.str();
      oss << std::endl << "  ";
      hex.str("");
      asc.str("");
    }
  }
  auto h = hex.str();
  if ( h.length() < 24) h.append(24 - h.length(), ' ');
  oss << h;
  oss << " | " << asc.str() << std::endl;

  return oss.str();
}

const std::vector<uint8_t> &
TapeDataBlock::Data() const {
  return bytes_;
}
