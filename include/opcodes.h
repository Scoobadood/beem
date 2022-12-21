//
// Created by Dave Durbin on 1/12/2022.
//

#ifndef CPU_OPCODES_H_
#define CPU_OPCODES_H_

#include <map>
#include <vector>

#include "opcodes.h"
#include "cpu.h"

using Operation = std::function<void(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk)>;

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
    Relative, ZeroPage,ZeroPageIndexedX, ZeroPageIndexedY
  } addressing_mode;
  Operation operation;
};

extern const std::map<uint8_t, OpCode> codes;

#endif //CPU_OPCODES_H_
