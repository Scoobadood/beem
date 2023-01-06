#include "disassembler.h"
#include "opcodes.h"

#include <spdlog/spdlog-inl.h>

Operation Disassembler::disassemble_one(//
    const std::vector<uint8_t> & memory //
    , uint16_t & offset //
    , uint8_t & err //
) const //
{
  if( offset>= memory.size()) {
    spdlog::warn( "offset ({}) out of range ({}) in disassemble_one()", offset, memory.size());
    err = 1;
    return {OpCode::unknown_opcode, 0xffff};
  }

  OpCode oc = OpCode::for_value(memory.at(offset));
  if( offset + oc.bytes > memory.size()) {
    spdlog::warn( "Arguments for opcode {} at {} have length {} and will be out of out of range {} in disassemble_one()",
                  oc.name, offset, oc.bytes, memory.size());
    err = 1;
    return {oc, 0xffff};
  }

  uint16_t data = 0;
  if( oc.bytes > 1) data = memory[offset + 1];
  if( oc.bytes > 2) data = (data ) | (memory[offset + 2] << 8);

  offset += oc.bytes;

  err = 0;
  return {oc, data};
}

std::vector<Operation> Disassembler::disassemble_all( //
    const std::vector<uint8_t> & memory //
    , uint16_t & offset //
    , uint8_t & err //
    ) const //
{
  using namespace std;

  vector<Operation> operations;
  while( offset < memory.size() - 1) {
    operations.emplace_back(disassemble_one(memory, offset, err));
    if( err ) {
      break;
    }
  }

  return operations;
}
