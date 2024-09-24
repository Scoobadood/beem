#ifndef M6502_DEBUGGER_SRC_DISASSEMBLER_H_
#define M6502_DEBUGGER_SRC_DISASSEMBLER_H_

#include <6502/opcodes.h>

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
                            uint8_t &err) const;
  Operation disassemble_one(const uint8_t * memory,
                            uint32_t length,
                            uint16_t &offset,
                            uint8_t &err) const;
  std::vector<Operation> disassemble_all(const std::vector<uint8_t> &memory,
                                         uint16_t &offset,
                                         uint8_t &err);

  inline void set_base_address(uint16_t base_address) {base_address_ = base_address;}

private:
  /* When disassembling data, first byte is assumed at this address */
  uint16_t base_address_;
};

#endif //M6502_DEBUGGER_SRC_DISASSEMBLER_H_
