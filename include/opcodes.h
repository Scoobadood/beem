//
// Created by Dave Durbin on 1/12/2022.
//

#ifndef CPU_OPCODES_H_
#define CPU_OPCODES_H_

#include <map>
#include <utility>
#include <vector>

#include "opcodes.h"
#include "cpu.h"
#include "memory.h"

using Operation = std::function<void(Cpu &cpu, Memory &memory, uint64_t &clk)>;

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
  Operation operation;
  OpCode(uint8_t bytes, uint8_t cycles, bool page_affected, std::string name,
         AddressingMode addressing_mode, Operation operation) //
      : bytes{bytes} //
      , cycles{cycles} //
      , page_affected{page_affected} //
      , name{std::move(name)} //
      , addressing_mode{addressing_mode} //
      , operation{std::move(operation)} //
  {}
};

extern const std::map<uint8_t, OpCode> codes;

#endif //CPU_OPCODES_H_
