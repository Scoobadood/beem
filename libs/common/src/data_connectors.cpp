#include "data_connectors.h"

#include <set>

void notify_subscribers(const std::set<data_subscriber_8_bit_ptr> &subs,
                        uint8_t value,
                        uint8_t bits_changed
                        ) {
  for (auto &sub: subs) {
    const auto mask = sub->mask();
    const auto bits_to_send = mask & bits_changed;
    auto test_bit = 0x01;
    for (auto i = 0; i < 8; ++i) {
      if (bits_to_send & test_bit) {
        sub->set_data(i, (value & test_bit));
      }
      test_bit <<= 1;
    }
  }
}

