#include "disassembler.h"
#include "opcodes.h"

#include <spdlog/spdlog-inl.h>

Disassembler::Disassembler() = default;

Operation Disassembler::disassemble_one(//
    const std::vector<uint8_t> &memory //
    , uint16_t &offset //
    , uint8_t &err //
) //
{
  if (offset >= memory.size()) {
    spdlog::warn("offset ({}) out of range ({}) in disassemble_one()", offset, memory.size());
    err = 1;
    return {0xffff, OpCode::unknown_opcode, 0xffff};
  }

  OpCode oc = OpCode::for_value(memory.at(offset));
  if (offset + oc.bytes > memory.size()) {
    spdlog::warn("Arguments for opcode {} at {} have length {} and will be out of out of range {} in disassemble_one()",
                 oc.name, offset, oc.bytes, memory.size());
    err = 1;
    return {offset, oc, 0xffff};
  }

  uint16_t data = 0;
  if (oc.bytes > 1) data = memory[offset + 1];
  if (oc.bytes > 2) data = (data) | (memory[offset + 2] << 8);

  auto addr = offset;
  offset  += oc.bytes;

  err = 0;
  return {addr, oc, data};
}

std::vector<Operation> Disassembler::disassemble_all( //
    const std::vector<uint8_t> &memory //
    , uint16_t &offset //
    , uint8_t &err //
) //
{
  using namespace std;

  vector<Operation> operations;
  err = 0;
  while (true) {
    auto last_offset = offset;
    auto op = disassemble_one(memory, offset, err);
    if (err) {
      break;
    }
    operations.emplace_back(op);
    if( offset < last_offset) {
      break;
    }
  }

  return operations;
}
