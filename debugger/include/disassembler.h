#ifndef M6502_DEBUGGER_SRC_DISASSEMBLER_H_
#define M6502_DEBUGGER_SRC_DISASSEMBLER_H_

#include "opcodes.h"

#include <utility>
#include <vector>

struct Operation {
  const OpCode opcode;
  const uint16_t data;
  Operation(OpCode op_code, const uint16_t data) : opcode{std::move(op_code)}, data{data} {}
};

class Disassembler {
 public:
  Operation disassemble_one(const std::vector<uint8_t> & memory,
                            uint16_t & offset,
                            uint8_t & err) const;
  std::vector<Operation> disassemble_all(const std::vector<uint8_t> &memory,
                                         uint16_t & offset,
                                         uint8_t & err) const;
};

#endif //M6502_DEBUGGER_SRC_DISASSEMBLER_H_
