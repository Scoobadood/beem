//
// Created by Dave Durbin on 11/1/2023.
//

#include "IC32Latch.h"

IC32Latch::IC32Latch() : latched_value_{0xff} {
  src_ = std::make_shared<data_subscriber_8_bit>(0x0f);
}

void IC32Latch::tick() {
  if (!src_->data_changed()) {
    return;
  }
  auto data = src_->data();

  // Work out which pin to modify
  uint8_t pin_number = data & 0x07;
  uint8_t mask = 1 << pin_number;

  if (data & 0x08) {
    latched_value_ |= mask;
  } else {
    latched_value_ &= ~mask;
  }
  notify_subscribers(subscribers_, latched_value_, mask);
}

void IC32Latch::subscribe(const data_subscriber_8_bit_ptr &subscriber) {
  subscribers_.emplace(subscriber);
}
