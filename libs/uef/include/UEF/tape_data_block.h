#ifndef LIBS_UEF__START_STOP_BIT_CHUNK_H_
#define LIBS_UEF__START_STOP_BIT_CHUNK_H_

#include "tape_data_chunk.h"

#include <vector>

/**
 * This chunk represents byte data stored on a cassette with Acorn's default start/stop bits
 * (which are not reproduced).
 *
 * 0_xxxxxxxx_1
 *
 * The least significant bit of the first byte is the first bit to appear on the tape, the
 * most significant the 8th, and so on. Hence bytewise values are the same as the bytes
 * stored on cassette.
 *
 * PSEUDO-CODE
 * while bytes remain in UEF chunk
 *   output a zero bit (the start bit)
 *   read a byte from the UEF chunk, store it to NewByte
 *   let InternalBitCount = 8
 *   while InternalBitCount > 0
 *     output least significant bit of NewByte
 *     shift NewByte right one position
 *     decrement InternalBitCount
 *   output a one bit (the stop bit)
 */
class TapeDataBlock : public TapeDataChunk {
 public:
  TapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~TapeDataBlock() override = default;

  const std::vector<uint8_t> &Data() const override;

  std::string Description() const override;
 private:
  std::vector<uint8_t> bytes_;
};
#endif // LIBS_UEF__START_STOP_BIT_CHUNK_H_
