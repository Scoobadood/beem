//
// Created by Dave Durbin on 11/1/2023.
//

#ifndef CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_
#define CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_

#include "6522_via.h"
#include "data_connectors.h"

#include <cstdint>
#include <set>
#include <spdlog/spdlog-inl.h>

class IC32Latch {
 public:
  IC32Latch();

  void tick();
  void subscribe(const data_subscriber_8_bit_ptr& subscriber);
  inline data_subscriber_8_bit_ptr src() { return src_; }

 private:
  data_subscriber_8_bit_ptr src_;
  uint8_t latched_value_;
  std::set<data_subscriber_8_bit_ptr> subscribers_;
};

#endif //CHIPS_M6502_HARDWARE_IC32_LATCH_IC32LATCH_H_
