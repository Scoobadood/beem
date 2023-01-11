//
// Created by Dave Durbin on 11/1/2023.
//

#ifndef CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_
#define CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_

#include "6522_via.h"
#include "data_subscribers.h"

#include <cstdint>
#include <set>
#include <spdlog/spdlog-inl.h>

class IC32Latch {
 public:
  IC32Latch();

  void subscribe( data_subscriber_8_bit & subscriber);

  void operator()(uint8_t data);

 private:
  uint8_t latched_value_;
  std::set<data_subscriber_8_bit> subscribers_;
};

#endif //CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_
