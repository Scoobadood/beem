#ifndef LIBS_BEEB_INCLUDE_DISASSEMBLER_OPERATION_FORMATTER_H_
#define LIBS_BEEB_INCLUDE_DISASSEMBLER_OPERATION_FORMATTER_H_

#include <string>
#include "disassembler.h"

auto format_single_line(const Operation &op, const std::map<uint16_t, Symbol> &symbols) -> std::string;
auto address_or_label(uint16_t data, uint16_t data_width, const std::map<uint16_t, Symbol> &symbols) -> std::string;
auto format_flags(uint8_t flags) -> std::string;
auto format_args(const Operation &op, const std::map<uint16_t, Symbol> &symbols) -> std::string;

#endif // LIBS_BEEB_INCLUDE_DISASSEMBLER_OPERATION_FORMATTER_H_
