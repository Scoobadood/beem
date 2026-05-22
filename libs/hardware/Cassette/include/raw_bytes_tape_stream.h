#pragma once

#include "i_tape_stream.h"
#include <vector>
#include <cstdint>

/**
 * Simplest ITapeStream implementation.
 *
 * Wraps a vector<uint8_t> and serialises each byte as a standard 8N1 frame:
 *   bit 0  : start bit (0)
 *   bits 1-8: data bits, LSB first
 *   bit 9  : stop bit (1)
 *
 * at_carrier() returns true while data remains, false after end_of_tape().
 * next_bit() returns mark (1) once the tape is exhausted.
 *
 * The recording path accumulates written bits back into bytes (write_bit /
 * recorded_bytes), allowing round-trip testing.
 */
class RawBytesTapeStream : public ITapeStream {
 public:
  explicit RawBytesTapeStream(std::vector<uint8_t> data);

  // ITapeStream
  bool next_bit() override;
  void write_bit(bool bit) override;
  bool at_carrier() override;
  bool end_of_tape() const override;

  // Access bytes accumulated via write_bit().
  const std::vector<uint8_t>& recorded_bytes() const { return recorded_; }

 private:
  // Playback state
  std::vector<uint8_t> data_;
  size_t tx_byte_{0};   // index of byte currently being transmitted
  int    tx_bit_{0};    // 0=start, 1-8=data bits, 9=stop

  // Recording state
  std::vector<uint8_t> recorded_;
  int  rx_state_{-1};   // -1=waiting for start, 0-7=data bits, 8=stop
  uint8_t rx_byte_{0};
};
