#ifndef M6502_DEBUGGER_SRC_DISASSEMBLER_H_
#define M6502_DEBUGGER_SRC_DISASSEMBLER_H_

#include "opcodes.h"

#include <utility>
#include <vector>

struct Operation {
  const uint16_t address;
  const OpCode opcode;
  const uint16_t data;
  Operation(uint16_t address, OpCode op_code, const uint16_t data) //
      : address{address} //
      , opcode{std::move(op_code)} //
      , data{data} //
  {}
};

class Disassembler {
 public:
  Disassembler();
  Operation disassemble_one(const std::vector<uint8_t> &memory,
                            uint16_t &offset,
                            uint8_t &err);
  std::vector<Operation> disassemble_all(const std::vector<uint8_t> &memory,
                                         uint16_t &offset,
                                         uint8_t &err);
};

#endif //M6502_DEBUGGER_SRC_DISASSEMBLER_H_
