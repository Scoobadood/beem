#ifndef LIBS_UEF_EXPLICIT_TAPE_DATA_CHUNK_H_
#define LIBS_UEF_EXPLICIT_TAPE_DATA_CHUNK_H_

#include "tape_data_chunk.h"
#include <vector>

/**
    Chunk &0102 - explicit tape data block
    Chunk &0102 is a raw representation of data bits stored on cassette. Unlike chunk &0100 there are no
    implicit start/stop bits. The first byte of this chunk is used to calculate chunk length at the bit
    level. Only the first (chunk length * 8) - (value of first byte) bits are used in this chunk.

    Bit ordering is as per &0100, so the least significant bit of any byte is the first bit on the tape.
    PSEUDO-CODE

   compute bit count for chunk - get chunk length, multiply it by 8 and subtract the value of the first byte.
   Store it to BitCount
   store zero to CurrentBit
   while CurrentBit < BitCount
     if (CurrentBit mod 8) = 0, read a new data byte from the chunk to NewByte
       output the least significant bit of NewByte
       shift NewByte right one position
       increment CurrentBit
       */

class ExplicitTapeDataBlock : public TapeDataChunk {
 public:
  ExplicitTapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~ExplicitTapeDataBlock() override = default;

  const std::vector<uint8_t> &Data() const override;
  uint32_t NumBits() const { return num_bits_; }

  std::string Description() const override;
 private:
  uint32_t num_bits_;
  std::vector<uint8_t> bytes_;
};

#endif // LIBS_UEF_EXPLICIT_TAPE_DATA_CHUNK_H_
