//
// Created by Dave Durbin on 11/1/2023.
//

#include "IC32Latch.h"

IC32Latch::IC32Latch() : latched_value_{0x0} {}

void IC32Latch::subscribe(data_subscriber_8_bit &subscriber) {
  subscribers_.emplace(subscriber);
}

void IC32Latch::operator()(uint8_t data) {
  // Work out which pin to modify
  uint8_t pin_number = data & 0x07;
  uint8_t mask = 1 << pin_number;

  auto old_value = latched_value_;
  if (data & 0x08) {
    latched_value_ |= mask;
  } else {
    latched_value_ &= ~mask;
  }

  // If the value changed, comment.
  // And push it out to subscribers.
  if (latched_value_ != old_value) {
    for (const auto &sub: subscribers_) {
      (*sub)(latched_value_);
    }

    switch (pin_number) {
      case 1:
        if (latched_value_ & 0x02) spdlog::info("Speech processor READ select enabled");
        else spdlog::info("Speech processor READ select disabled");
        break;
      case 2:
        if (latched_value_ & 0x04) spdlog::info("Speech processor WRITE select enabled");
        else spdlog::info("Speech processor WRITE select disabled");
        break;
      case 3:
        if (latched_value_ & 0x08) spdlog::info("Keyboard write enabled");
        else spdlog::info("Keyboard write disabled");
        break;
      case 4:
      case 5:
        switch ((latched_value_ & 0x30) >> 4) {
          case 0x00:spdlog::info("Screen start &4000");
            break;
          case 0x01:spdlog::info("Screen start &6000");
            break;
          case 0x02:spdlog::info("Screen start &3000");
            break;
          case 0x03:spdlog::info("Screen start &5800");
            break;
        }
        break;

      case 6:
        if (latched_value_ & 0x40) spdlog::info("CAPS lock on");
        else spdlog::info("CAPS lock off");
        break;
      case 7:
        if (latched_value_ & 0x80) spdlog::info("SHIFT lock on");
        else spdlog::info("SHIFT lock off");
        break;

      default:break;
    }
  }
}