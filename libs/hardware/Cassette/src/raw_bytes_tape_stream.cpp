#include "raw_bytes_tape_stream.h"

RawBytesTapeStream::RawBytesTapeStream(std::vector<uint8_t> data)
    : data_(std::move(data)) {}

bool RawBytesTapeStream::end_of_tape() const {
  return tx_byte_ >= data_.size();
}

bool RawBytesTapeStream::at_carrier() {
  return !end_of_tape();
}

bool RawBytesTapeStream::next_bit() {
  if (end_of_tape()) return true;  // mark / idle

  bool bit;
  if (tx_bit_ == 0) {
    bit = false;  // start bit
  } else if (tx_bit_ <= 8) {
    bit = (data_[tx_byte_] >> (tx_bit_ - 1)) & 1;  // data bits, LSB first
  } else {
    bit = true;   // stop bit
  }

  if (++tx_bit_ == 10) {
    tx_bit_ = 0;
    ++tx_byte_;
  }

  return bit;
}

void RawBytesTapeStream::write_bit(bool bit) {
  if (rx_state_ == -1) {
    // Waiting for start bit (low)
    if (!bit) {
      rx_state_ = 0;
      rx_byte_  = 0;
    }
    return;
  }

  if (rx_state_ < 8) {
    // Data bits, LSB first
    if (bit) rx_byte_ |= (1u << rx_state_);
    ++rx_state_;
    return;
  }

  // Stop bit — commit byte regardless of stop-bit value
  recorded_.push_back(rx_byte_);
  rx_state_ = -1;
  rx_byte_  = 0;
}
