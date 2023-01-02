//
// Created by Dave Durbin on 30/12/2022.
//

#ifndef M6502_INCLUDE_MEMORY_H_
#define M6502_INCLUDE_MEMORY_H_

#include "via.h"

#include <vector>
#include <iterator>
#include <fstream>

class Memory {
 public:
  explicit Memory(uint16_t sz);

  explicit Memory(std::ifstream &f);

  uint8_t at(uint16_t addr) const;

  void set(uint16_t addr, uint8_t arg);

  void insert(uint16_t offset, std::vector<uint8_t> &data);

 private:
  void handle_rom_writes(uint16_t addr, uint8_t arg);

  SystemVia system_via_;
  std::vector<uint8_t> memory_;
  uint32_t size_;
};

#endif //M6502_INCLUDE_MEMORY_H_
