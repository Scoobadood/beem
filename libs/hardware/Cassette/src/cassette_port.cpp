#include "cassette_port.h"

bool CassettePort::rx_data() {
  if (!motor_on_ || !stream_) return true;  // mark / idle
  return stream_->next_bit();
}

bool CassettePort::has_carrier() {
  if (!motor_on_ || !stream_) return false;
  return stream_->at_carrier();
}

void CassettePort::tx_bit(bool bit) {
  if (!motor_on_ || !stream_) return;
  stream_->write_bit(bit);
}

void CassettePort::set_motor(bool on) {
  motor_on_ = on;
}
