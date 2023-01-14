//
// Created by Dave Durbin on 30/12/2022.
//

#include "bus.h"
#include "rom.h"

#include <iterator>
#include <fstream>

Rom::Rom(std::ifstream &f) {
  using namespace std;
  memory_ = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

void Rom::tick(Bus & bus) {
  auto addr = bus.get_address();
  if( addr < 0x8000 || addr >= 0xbfff) return;

  addr -= 0x8000;

  // Read and write memory
  if (bus.tst_RW()) {
    auto data = memory_.at(addr);
    bus.set_data(data);
  } else {
    auto data = bus.get_data();
    memory_.at(addr) = data;
  }
}

/**
 * Return the contents of the given address.
 * @param addr The address
 * @return The contents
 */
uint8_t Rom::at(uint16_t addr) const {
  return memory_.at(addr);
}


