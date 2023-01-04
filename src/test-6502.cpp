#include "m6502.h"
#include "memory.h"

#include <fstream>
#include <iostream>
#include <vector>

int main() {
  using namespace std;

  // Load bin file
  ifstream f("data/6502_functional_test.bin", ios::binary);
  if (!f.is_open()) {
    cerr << "File read failed" << endl;
    return 0;
  }
  auto memory = Memory(f);
  f.close();

  M6502 cpu;

  // Histoyr buffer for PC
  uint32_t buffer_size = 50;
  uint32_t dup_pc_count = 0;
  uint32_t buffer_idx = 0;
  std::vector<uint16_t> pc_history(buffer_size, 0);

  // Pull reset low
  uint64_t pins = 0;
  while (true) {
    pins = cpu.tick(pins);
    set_RST(pins);

    // Make sure code execution starts at 0x400, not reset vector
    auto addr = get_address(pins);
    // Special case reset
    if (addr == 0xfffc) {
      set_data(pins, 0);
      continue;
    } else if (addr == 0xfffd) {
      set_data(pins, 4);
      continue;
    }

    // Read and write memory
    if (tst_RW(pins)) {
      auto data = memory.at(addr);
      set_data(pins, data);
    } else {
      auto data = get_data(pins);
      memory.set(addr, data);
    }

    // Log PC to history buffer
    auto pc = cpu.pc();
    if (pc == pc_history.at(buffer_idx)) {
      dup_pc_count++;
      if (dup_pc_count == 8) {
        if (pc == 0x3469) {
          spdlog::info("All tests passed!");
          return EXIT_SUCCESS;
        } else {
          spdlog::critical("Stuck in a loop at PC: 0x{:04x}", pc);
          for (auto i = 0; i < buffer_size; ++i) {
            spdlog::info("{:04x}", pc_history.at((buffer_idx + i) % buffer_size));
          }
          return EXIT_FAILURE;
        }
      }
    } else {
      dup_pc_count = 0;
      buffer_idx = (buffer_idx + 1) % buffer_size;
      pc_history.at(buffer_idx) = pc;
    }
  }
}
