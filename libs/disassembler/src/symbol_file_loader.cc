#include "symbol_file_loader.h"

#include <fstream>
#include <spdlog/spdlog.h>

void trim_in_place(std::string& str) {
  // Remove leading whitespace
  str.erase(str.begin(), std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
    return std::isspace(c);
  }));

  // Remove trailing whitespace
  str.erase(std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
    return std::isspace(c);
  }).base(), str.end());
}

std::map<uint16_t, Symbol> load_symbols_from_file(const std::string &file_name) {
  if (file_name.empty()) {
    spdlog::error("File name is empty in load_symbols");
    return {};
  }
  std::ifstream file{file_name, std::ios::in};
  if (file.fail()) {
    spdlog::error("Error reading file {} ", file_name);
    return {};
  }

  std::string line;
  std::map<uint16_t, Symbol> symbols;
  while (getline(file, line)) {
    // Split into address and symbol
    auto idx = line.find(' ');
    if (idx != -1) {
      auto addr_txt = line.substr(0, idx);
      trim_in_place(addr_txt);
      try {
        auto addr = (uint16_t) std::stoi(addr_txt, nullptr, 16);
        auto symbol = line.substr(idx);
        trim_in_place(symbol);
        symbols.emplace(addr, Symbol{symbol, addr});
      }  catch (const std::invalid_argument&) {
        spdlog::error( "Invalid address '{}' in symbol file {}", addr_txt, file_name);
      } catch (const std::out_of_range&) {
        spdlog::error( "Address '{}' out of range in  symbol file {}", addr_txt, file_name);
      }
    }
  }
  file.close();
  return symbols;
}