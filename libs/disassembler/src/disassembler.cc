#include "disassembler.h"

#include <spdlog/spdlog-inl.h>

Disassembler::Disassembler()
    : base_address_{0} //
{}

Operation Disassembler::disassemble_one(//
    const std::vector<uint8_t> &memory //
    , uint16_t &offset //
    , uint8_t &err //
) const //
{
  return disassemble_one(memory.data(), memory.size(), offset, err);
}

Operation Disassembler::disassemble_one(const uint8_t * memory,
                          uint32_t length,
                          uint16_t &offset,
                          uint8_t &err) const
{
  if (offset >= length) {
    spdlog::warn("offset ({}) out of range ({}) in disassemble_one()", offset, length);
    err = 1;
    return {0xffff, OpCode::unknown_opcode, 0xffff};
  }
  OpCode oc = OpCode::for_value(memory[offset]);
  if (offset + oc.bytes > length) {
    spdlog::warn(
        "Arguments for opcode {} at_bus {} have length {} and will be out of out of range {} in disassemble_one()",
        oc.name, offset, oc.bytes, length);
    err = 1;
    return {offset, oc, 0xffff};
  }

  uint16_t data = 0;
  if (oc.bytes > 1) data = memory[offset + 1];
  if (oc.bytes > 2) data = (data) | (memory[offset + 2] << 8);

  auto addr = (uint16_t) (offset + base_address_ & 0xffff);
  offset += oc.bytes;

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
  if (memory.empty()) return {};

  vector<Operation> operations;
  err = 0;
  do {
    auto last_offset = offset;
    auto op = disassemble_one(memory, offset, err);
    if (err) {
      break;
    }
    operations.emplace_back(op);
    if (offset < last_offset) {
      break;
    }
  } while (true);

  return operations;
}
