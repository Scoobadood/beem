//
// Created by Dave Durbin on 11/1/2023.
//

#ifndef CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_
#define CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_

#include "6522_via.h"

#include <cstdint>
#include <spdlog/spdlog-inl.h>

class IC32Latch {
 public:
  IC32Latch(Via *via) : via_{via}, value_{0} {}
  Via *via_;
  uint8_t value_;
  void tick() {
    if (via_) {
      auto inp = via_->poll_port_b(0x0f);
      auto bit = inp & 0x07;
      auto mask = 1 << bit;

      auto old_value = value_;
      if (inp & 0x08) {
        value_ |= mask;
      } else {
        value_ &= ~mask;
      }

      if (value_ != old_value) {
        switch (bit) {
          case 0:
            if (value_ & 0x01) spdlog::info("Sound generator enabled");
            else spdlog::info("Sound generator disabled");
            break;
          case 1:
            if (value_ & 0x02) spdlog::info("Speech processor READ select enabled");
            else spdlog::info("Speech processor READ select disabled");
            break;
          case 2:
            if (value_ & 0x04) spdlog::info("Speech processor WRITE select enabled");
            else spdlog::info("Speech processor WRITE select disabled");
            break;
          case 3:
            if (value_ & 0x08) spdlog::info("Keyboard write enabled");
            else spdlog::info("Keyboard write disabled");
            break;
          case 4:
          case 5:
            switch ((value_ & 0x03) >> 4) {
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
            if (value_ & 0x40) spdlog::info("CAPS lock on");
            else spdlog::info("CAPS lock off");
            break;
          case 7:
            if (value_ & 0x80) spdlog::info("SHIFT lock on");
            else spdlog::info("SHIFT lock off");
            break;
        }
      }
    }
  }
};

#endif //CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_
