#ifndef LIBS_UEF_UNKNOWN_CHUNK_H_
#define LIBS_UEF_UNKNOWN_CHUNK_H_

#include "chunk.h"

class UnknownChunk : public Chunk {
 public:
  explicit UnknownChunk(uint16_t chunk_id);
  ~UnknownChunk() override = default;
  std::string Description() const override;
};
#endif // LIBS_UEF_UNKNOWN_CHUNK_H_