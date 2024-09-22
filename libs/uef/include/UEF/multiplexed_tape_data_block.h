#ifndef LIBS_UEF_MULTIPLEXED_TAPE_DATA_BLOCK_H_
#define LIBS_UEF_MULTIPLEXED_TAPE_DATA_BLOCK_H_

#include "tape_data_chunk.h"
#include <vector>

/**
 * Chunk &0101 - multiplexed data block
 * The chunks that store meaningful tape data that may be multiplexed are &0100 and &0102.
 * If either of these is immediately followed by a &0101 chunk then that chunk contains
 * exactly the same information as its predecessor, except that the data fields are expanded
 * to contain multiplexed data.
 * If this chunk appears after any chunk that is neither &0100 nor &0102 then it has no meaning
 * and should be ignored.
 *
 * Older UEFs may use &0103 as a synonym for &0101.
 */

class MultiplexedTapeDataBlock : public TapeDataChunk {
 public:
  MultiplexedTapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~MultiplexedTapeDataBlock() override = default;

  const std::vector<uint8_t> &Data() const override;

  std::string Description() const override;
 private:
  std::vector<uint8_t> bytes_;
};
#endif // LIBS_UEF_MULTIPLEXED_TAPE_DATA_BLOCK_H_
