//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef M6502_INCLUDE_CYCLE_HANDLER_H_
#define M6502_INCLUDE_CYCLE_HANDLER_H_

#include "m6502.h"

#include <functional>
#include <map>

using CycleHandler = std::function<void(M6502 *, uint64_t &)>;

CycleHandler cycle_handler(uint16_t ir);
struct OpCode {
  uint8_t bytes;
  uint8_t cycles;
  bool page_affected;
  std::string name;
  enum AddressingMode {
    Accumulator,
    Absolute, AbsoluteIndexedX, AbsoluteIndexedY,
    Immediate, Implied,
    Indirect, IndirectIndexedX, IndirectIndexedY,
    Relative, ZeroPage, ZeroPageIndexedX, ZeroPageIndexedY
  } addressing_mode;
  OpCode(uint8_t bytes, uint8_t cycles, bool page_affected, std::string name,
         AddressingMode addressing_mode) //
      : bytes{bytes} //
      , cycles{cycles} //
      , page_affected{page_affected} //
      , name{std::move(name)} //
      , addressing_mode{addressing_mode} //
  {}
  std::string to_string() const;
};
extern const std::map<uint8_t, OpCode> op_codes;


#endif //M6502_INCLUDE_CYCLE_HANDLER_H_
