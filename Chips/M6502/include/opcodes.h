//
// Created by Dave Durbin on 6/1/2023.
//

#ifndef CHIPS_M6502_CHIPS_M6502_INCLUDE_OPCODES_H_
#define CHIPS_M6502_CHIPS_M6502_INCLUDE_OPCODES_H_

#include <string>

struct OpCode {
  uint8_t hex;
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

  OpCode(uint8_t hex, uint8_t bytes, uint8_t cycles, bool page_affected, std::string name,
         AddressingMode addressing_mode) //
      : hex{hex} //
      , bytes{bytes} //
      , cycles{cycles} //
      , page_affected{page_affected} //
      , name{std::move(name)} //
      , addressing_mode{addressing_mode} //
  {}

  static const OpCode unknown_opcode;

  static OpCode for_value(uint8_t value);
};

OpCode opcode_for(uint8_t value);
#endif //CHIPS_M6502_CHIPS_M6502_INCLUDE_OPCODES_H_
