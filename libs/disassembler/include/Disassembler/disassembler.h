#ifndef M6502_DEBUGGER_SRC_DISASSEMBLER_H_
#define M6502_DEBUGGER_SRC_DISASSEMBLER_H_

#include <6502/opcodes.h>

#include <utility>
#include <vector>
#include "symbol_file_loader.h"

struct Operation {
  const std::string label;
  const uint16_t address;
  const OpCode opcode;
  const uint16_t data;
  Operation(uint16_t address, OpCode op_code, const uint16_t data) //
      : address{address} //
      , opcode{std::move(op_code)} //
      , data{data} //
  {}
  Operation(std::string label, uint16_t address, OpCode op_code, const uint16_t data) //
      : label{std::move(label)} //
      , address{address} //
      , opcode{std::move(op_code)} //
      , data{data} //
  {}
};

class Disassembler {
 public:
  Disassembler();
  ~Disassembler();
  auto disassemble_one(const std::vector<uint8_t> &memory,
                            uint16_t &offset,
                            uint8_t &err) const -> Operation;
  auto disassemble_one(const uint8_t *memory,
                            uint32_t length,
                            uint16_t &offset,
                            uint8_t &err) const -> Operation;
  auto disassemble_all(const std::vector<uint8_t> &memory,
                                         uint16_t &offset,
                                         uint8_t &err) -> std::vector<Operation>;

  inline void set_base_address(uint16_t base_address) { base_address_ = base_address; }

  void set_symbols(const std::map<uint16_t, Symbol> &symbols);
  [[nodiscard]] auto symbols() const -> const std::map<uint16_t, Symbol> & { return symbols_;}

 private:
  /* When disassembling data, first byte is assumed at this address */
  uint16_t base_address_;

  std::map<uint16_t, Symbol> symbols_;
};

#endif //M6502_DEBUGGER_SRC_DISASSEMBLER_H_
