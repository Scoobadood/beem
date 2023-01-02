#include <iostream>

#include <vector>
#include <fstream>
#include <sstream>

#include "cpu.h"
#include "opcodes.h"

std::string dump(const Cpu &cpu, uint32_t clk) {
  using namespace std;

  stringstream oss;

  oss << "PC: " << hex << setfill('0') << setw(4) << cpu.pc_;
  oss << "  SP: " << hex << setfill('0') << setw(2) << cpu.stack_pointer_;
  oss << "  ST: "
      << (cpu.minus() ? 'N' : 'n')
      << (cpu.is_overflow() ? 'V' : 'v')
      << '_'
      << '1'
      << (cpu.is_decimal() ? 'D' : 'd')
      << (cpu.is_interrupt() ? 'I' : 'i')
      << (cpu.zero() ? 'Z' : 'z')
      << (cpu.carry() ? 'C' : 'c');

  oss << "  A: " << (cpu.accumulator_ & 0x80 ? '1' : '0')
      << (cpu.accumulator_ & 0x40 ? '1' : '0')
      << (cpu.accumulator_ & 0x20 ? '1' : '0')
      << (cpu.accumulator_ & 0x10 ? '1' : '0')
      << (cpu.accumulator_ & 0x08 ? '1' : '0')
      << (cpu.accumulator_ & 0x04 ? '1' : '0')
      << (cpu.accumulator_ & 0x02 ? '1' : '0')
      << (cpu.accumulator_ & 0x01 ? '1' : '0')
      << "  (" << std::hex << setfill('0') << setw(2) << cpu.accumulator_ << ")";

  oss << "  X: " << (cpu.x_reg_ & 0x80 ? '1' : '0')
      << (cpu.x_reg_ & 0x40 ? '1' : '0')
      << (cpu.x_reg_ & 0x20 ? '1' : '0')
      << (cpu.x_reg_ & 0x10 ? '1' : '0')
      << (cpu.x_reg_ & 0x08 ? '1' : '0')
      << (cpu.x_reg_ & 0x04 ? '1' : '0')
      << (cpu.x_reg_ & 0x02 ? '1' : '0')
      << (cpu.x_reg_ & 0x01 ? '1' : '0')
      << "  (" << std::hex << setfill('0') << setw(2) << cpu.x_reg_ << ")";

  oss << "   Y: " << (cpu.y_reg_ & 0x80 ? '1' : '0')
      << (cpu.y_reg_ & 0x40 ? '1' : '0')
      << (cpu.y_reg_ & 0x20 ? '1' : '0')
      << (cpu.y_reg_ & 0x10 ? '1' : '0')
      << (cpu.y_reg_ & 0x08 ? '1' : '0')
      << (cpu.y_reg_ & 0x04 ? '1' : '0')
      << (cpu.y_reg_ & 0x02 ? '1' : '0')
      << (cpu.y_reg_ & 0x01 ? '1' : '0')
      << "  (" << std::hex << setfill('0') << setw(2) << cpu.y_reg_ << ")";

  oss << "  CLK: " << dec << setfill('0') << setw(8) << clk;

  oss << endl;
  return oss.str();
}

std::string dump_instruction(const Cpu &cpu,
                             const OpCode &instruction) {
  using namespace std;

  stringstream oss;

  oss << "PC: " << hex << setfill('0') << setw(4) << cpu.pc_;
  oss << "  " << instruction.name;
  oss << setfill(' ') << setw(7);
  switch (instruction.addressing_mode) {
    case OpCode::Accumulator: oss << "Acc";
      break;
    case OpCode::Absolute: oss << "Abs";
      break;
    case OpCode::AbsoluteIndexedX: oss << "Abs,X";
      break;
    case OpCode::AbsoluteIndexedY: oss << "Abs,Y";
      break;
    case OpCode::Immediate: oss << "Imm";
      break;
    case OpCode::Implied: oss << "";
      break;
    case OpCode::Indirect: oss << "Ind";
      break;
    case OpCode::IndirectIndexedX: oss << "Ind,X";
      break;
    case OpCode::IndirectIndexedY: oss << "Ind,Y";
      break;
    case OpCode::Relative: oss << "Rel";
      break;
    case OpCode::ZeroPage: oss << "Zpg";
      break;
    case OpCode::ZeroPageIndexedX: oss << "Zpg,X";
      break;
    case OpCode::ZeroPageIndexedY: oss << "Zpg,Y";
      break;
    default: oss << "ERR";
      break;
  }
  oss << "    | ";
  return oss.str();
}

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

  uint32_t buffsize = 20;
  std::vector<std::string> history;
  history.resize(buffsize, "");
  uint32_t next = 0;

  // Set PC
  Cpu cpu;
  cpu.stack_pointer_ = 0xff;
  cpu.pc_ = 0x400;
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
    auto str1 = dump_instruction(cpu, iter->second);
    cpu.pc_++;
    iter->second.operation(cpu, memory, clk);
    auto str2 = dump(cpu, clk);

    // Stash to history buffer in case of fail.
    history.at(next) = str1 + str2;
    next = (next + 1) % buffsize;


    // Handle fail
    if (start_pc == cpu.pc_) {
      if (cpu.pc_ == 0x3469) {
        cout << "All tests passed!";
        return EXIT_SUCCESS;
      }
      cout << "Stuck in a loop at PC: 0x" << hex << setw(4) << cpu.pc_ << endl;
      for (auto i = 0; i < buffsize; ++i) {
        cout << history.at((next + i) % buffsize);
      }
      return EXIT_FAILURE;
    }
  }
}
