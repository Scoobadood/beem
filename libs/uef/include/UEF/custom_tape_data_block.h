#ifndef LIBS_UEF_CUSTOM_TAPE_DATA_BLOCK_H_
#define LIBS_UEF_CUSTOM_TAPE_DATA_BLOCK_H_

#include "tape_data_chunk.h"
#include <vector>

/**
 * Chunk &0104 - defined tape format data block
 *
 * This chunk holds byte data with specified non-standard start/stop/parity bits. It is analogous to &0100
 * in that bytes of data are read from the UEF then packaged to produce the tape signal. Unlike &0100,
 * block packaging may include arbitrary stop and parity bits. Like &0100 blocks always have an implicit start bit.
 *
 * While processing this chunk, bytes are read from the source UEF and packaged into packets. The packet format
 * is defined by the first three bytes in the chunk.
 *
 * The first byte holds the number of data bits per packet, not counting start/stop/parity bits.
 *
 * The second byte holds the ascii code for 'N', 'E' or 'O', which specifies that parity is not present, even or odd.
 *
 * The third byte holds information concerning stop bits. If it is a positive number then it is a count of stop bits.
 * If it is a negative number then it is a negatived count of stop bits to which an extra short wave should be added.
 *
 * Positive numbers should be used wherever possible. Reproductions of original BBC and Electron material should only
 * produce positive numbers if correctly encoded.
 *
 * The total number of packets is equal to the chunk's length minus three. Bits in bytes are stored as in block
 * &0100, i.e. the least significant bit should appear on cassette first.
 *
 * Data blocks are always stored in the chunk as whole byte quantities. If the number of data bits is
 * seven then the most significant bits of all bytes in the chunk are unused and should be zero.
 *
 * Normal start bits should always be inserted into data, as per the implicit data chunk, &0100.
 *
 * For the BBC/Electron, the following formats may be encountered: 7E1, 7E2, 7O1, 7O2, 8E1, 8N2, 8O1.
 * Format 8N1 would produce the same output as chunk &0100.
 *
 * For the Atom, data format will usually be 8N-1.
 */

class CustomTapeDataBlock : public TapeDataChunk {
 public:
  enum Parity {
    NONE,
    ODD,
    EVEN
  };

  CustomTapeDataBlock(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~CustomTapeDataBlock() override = default;

  const std::vector<uint8_t> &Data() const override;
  std::string Description() const override;

 private:
  uint8_t bits_per_packet_;
  Parity parity_;
  uint8_t stop_bits_;
  bool include_short_wave_;
  std::vector<uint8_t> bytes_;
};

#endif // LIBS_UEF_CUSTOM_TAPE_DATA_BLOCK_H_
