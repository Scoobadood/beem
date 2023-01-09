//
// Created by Dave Durbin on 30/12/2022.
//

#include "memory.h"

#include <iterator>
#include <fstream>

#include "m6502.h"

Memory::Memory(uint32_t sz) {
  memory_.resize(sz, 0);
}

Memory::Memory(std::ifstream &f) {
  using namespace std;
  memory_ = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

bool is_memory_mapped(uint16_t addr) {
  // Avoid SHEILA JIM and FRED
  return (addr >= 0xfc00 && addr <= 0xfeff);
}

uint64_t Memory::tick(uint64_t pins) {
  auto addr = get_address(pins);
  if (is_memory_mapped(addr)) return pins;

  // Read and write memory
  if (tst_RW(pins)) {
    auto data = memory_.at(addr);
    set_data(pins, data);
  } else {
    auto data = get_data(pins);
    memory_.at(addr) = data;
  }
  return pins;
}

/**
 * Return the contents of the given address.
 * @param addr The address
 * @return The contents
 */
uint8_t Memory::at(uint16_t addr) const {
  return memory_.at(addr);
}

void Memory::insert(uint16_t offset, std::vector<uint8_t> &data) {
  auto wr_iter =memory_.begin() + offset;
  for(unsigned char & i : data) {
    *wr_iter++ = i;
  }
}



