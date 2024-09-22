#ifndef LIBS_UEF_ORIGIN_CHUNK_H_
#define LIBS_UEF_ORIGIN_CHUNK_H_

#include "chunk.h"

#include <cstdint>
#include <vector>

class OriginChunk : public Chunk {
 public:
  OriginChunk(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~OriginChunk() override = default;

  std::string Description() const override;

 private:
  std::vector<std::string> strings_;
};

#endif // LIBS_UEF_ORIGIN_CHUNK_H_
