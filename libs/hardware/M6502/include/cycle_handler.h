//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef M6502_INCLUDE_CYCLE_HANDLER_H_
#define M6502_INCLUDE_CYCLE_HANDLER_H_

#include "m6502.h"
#include "bus.h"

#include <functional>
#include <map>

using CycleHandler = std::function<void(M6502 *, Bus &)>;

CycleHandler cycle_handler(uint16_t ir);

#endif //M6502_INCLUDE_CYCLE_HANDLER_H_
