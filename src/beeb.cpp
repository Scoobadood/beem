#include <iostream>

#include <vector>
#include <fstream>

#include "cpu.h"
#include "opcodes.h"

int main() {
  using namespace std;

  // Load bin file
  ifstream f("data/os120", ios::binary);
  if (!f.is_open()) {
    cerr << "File read failed" << endl;
    return 0;
  }

  auto rom = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
  f.close();
  auto memory = Memory(65535);
  memory.insert(0xc000, rom);

  uint32_t buffsize = 20;
  std::vector<std::string> history;
  history.resize(buffsize, "");
  uint32_t next = 0;

  // Set PC
  Cpu cpu;
  cpu.stack_pointer_ = 0xff;
  cpu.pc_ = memory.at(0xfffc) + memory.at(0xfffd) * 256;
  uint64_t clk = 0;

  // Execute code.
  while (true) {
    auto start_pc = cpu.pc_;

    auto ins = memory.at(cpu.pc_);
    auto iter = codes.find(ins);
    if (iter == codes.end()) {
      cout << "Unrecognised opcode 0x" << hex << setfill('0') << setw(2) << (uint32_t) ins << " at PC: 0x" << setw(4)
           << cpu.pc_ << endl;
      break;
    }
    auto str1 = iter->second.to_string();
    cpu.pc_++;
    iter->second.operation(cpu, memory, clk);
    auto str2 = cpu.to_string();
    cout << "PC: " << hex << setfill('0') << setw(4) << cpu.pc_ << str1 << str2;
  }
}
