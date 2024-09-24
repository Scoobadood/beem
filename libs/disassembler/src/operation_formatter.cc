#include "operation_formatter.h"

#include <spdlog/fmt/fmt.h>

auto address_or_label(uint16_t data, uint16_t data_width, const std::map<uint16_t, Symbol> &symbols) -> std::string {
  auto symb_iter = symbols.find(data);
  if (symb_iter != symbols.end()) {
    return symb_iter->second.name;
  }
  return fmt::format("&{:0{width}x}", data, fmt::arg("width", data_width));
}

auto format_flags(uint8_t flags) -> std::string {
  return fmt::format("{}{}_{}{}{}{}{}",
                     flags & 0x80 ? 'N' : 'n',
                     flags & 0x40 ? 'V' : 'v',
                     flags & 0x10 ? 'B' : 'b',
                     flags & 0x08 ? 'D' : 'd',
                     flags & 0x04 ? 'I' : 'i',
                     flags & 0x02 ? 'Z' : 'z',
                     flags & 0x01 ? 'C' : 'c'
  );
}

auto format_args(const Operation &op, const std::map<uint16_t, Symbol> &symbols) -> std::string {
  switch (op.opcode.addressing_mode) {
    case OpCode::Implied:
      return "";
    case OpCode::Immediate:
      return fmt::format("#&{:02x}", op.data);
    case OpCode::Absolute:
      return address_or_label(op.data, op.opcode.bytes, symbols);
    case OpCode::IndirectIndexedX:
      return fmt::format("({},X)", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::IndirectIndexedY:
      return fmt::format("({}),Y", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::Indirect:
      return fmt::format("({})", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::AbsoluteIndexedX:
      return fmt::format("{},X", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::AbsoluteIndexedY:
      return fmt::format("{},Y", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::Accumulator:
      return "A";
    case OpCode::ZeroPage:
      return fmt::format("{}", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::ZeroPageIndexedY:
      return fmt::format("{},Y", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::ZeroPageIndexedX:
      return fmt::format("{},X", address_or_label(op.data, op.opcode.bytes, symbols));
    case OpCode::Relative:
      return fmt::format("{}", address_or_label(op.address + 2 + (int8_t) op.data, 2, symbols));
  }
}
/*
 * Format is:

 llllllllllllllllllll ooo arg
 */
auto format_single_line(const Operation &op, const std::map<uint16_t, Symbol> &symbols) -> std::string {
  auto arg = format_args(op, symbols);
  return fmt::format("{:20s} {:04x} {:4s} {:40s}",
                     op.label,
                     op.address,
                     op.opcode.name,
                     arg);
}
