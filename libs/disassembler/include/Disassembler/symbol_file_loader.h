#ifndef LIBS_DISASSEMBLER_INCLUDE_SYMBOL_FILE_LOADER_H_
#define LIBS_DISASSEMBLER_INCLUDE_SYMBOL_FILE_LOADER_H_

#include <string>
#include <map>

struct Symbol {
  std::string name;
  uint16_t address;
};

std::map<uint16_t, Symbol> load_symbols_from_file(const std::string& file_name );


#endif //LIBS_DISASSEMBLER_INCLUDE_SYMBOL_FILE_LOADER_H_
