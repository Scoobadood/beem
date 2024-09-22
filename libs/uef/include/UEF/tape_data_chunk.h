#ifndef LIBS_UEF_TAPE_DATA_CHUNK_H_
#define LIBS_UEF_TAPE_DATA_CHUNK_H_

#include "chunk.h"

/**
 * Tape chunks that contain data.
 * They support a Data() method which returns their source data
 * free of any start/stop bits etc.
 */
class TapeDataChunk : public Chunk {
 public:
  bool IsTapeDataChunk() const override { return true;}
  virtual const std::vector<uint8_t> &Data() const = 0;

 protected:
  explicit TapeDataChunk(const std::string &chunk_name)
      : Chunk(chunk_name) {}
  ~TapeDataChunk() override = default;


  std::string Description() const override = 0;
};

#endif // LIBS_UEF_TAPE_DATA_CHUNK_H_
