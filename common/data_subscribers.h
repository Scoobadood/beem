//
// Created by Dave Durbin on 11/1/2023.
//

#ifndef CHIPS_M6502_HARDWARE_COMMON_DATA_SUBSCRIBERS_H_
#define CHIPS_M6502_HARDWARE_COMMON_DATA_SUBSCRIBERS_H_

#include <functional>

using data_subscriber_8_bit = std::shared_ptr<std::function<void(uint8_t)>>;

#endif //CHIPS_M6502_HARDWARE_COMMON_DATA_SUBSCRIBERS_H_
