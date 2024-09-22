#include "origin_chunk.h"

#include <sstream>
#include <spdlog/spdlog.h>

OriginChunk::OriginChunk(std::unique_ptr<std::istream> &uef_stream,
                         uint32_t chunk_length)
    : Chunk{"Origin"}//
{
  // Contains a series of ASCII NUL terminated strings
  // Repeatedly read charts, check they're ascii, append to strings
  char c;
  std::ostringstream ss;
  for (auto i = 0; i < chunk_length; i++) {
    uef_stream->get(c);
    // Accept 0, 9 TAB, 10 LF, 32->127
    if (c == 0) {
      strings_.push_back(ss.str());
      ss.str("");
      ss.clear();
    } else if (c == 9 || c == 10 || c < 127) {
      ss << c;
    } else {
      spdlog::warn("Ignoring character {:02x} in Origin", c);
    }
  }
  auto last_str = ss.str();
  if (!last_str.empty()) {
    spdlog::warn("Final string in Origin is not terminated");
    strings_.push_back(last_str);
  }
}

std::string
OriginChunk::Description() const {
  std::ostringstream oss;
  oss << "Origin" << std::endl;
  for (const auto &s : strings_) {
    oss << "  " << s << std::endl;
  }
  return oss.str();
}