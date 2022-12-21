#include <iostream>

#include <vector>
#include <fstream>

#include "cpu.h"
#include "opcodes.h"

void dump(const Cpu &cpu, uint32_t clk) {
  using namespace std;

  cout << "PC: " << hex << setfill('0') << setw(4) << cpu.pc_;
  cout << "  SP: " << hex << setfill('0') << setw(2) << cpu.stack_pointer_;
  cout << "  ST: "
       << (cpu.minus() ? 'N' : 'n')
       << (cpu.is_overflow() ? 'V' : 'v')
       << '_'
       << '1'
       << (cpu.is_decimal() ? 'D' : 'd')
       << (cpu.is_interrupt() ? 'I' : 'i')
       << (cpu.zero() ? 'Z' : 'z')
       << (cpu.carry() ? 'C' : 'c');

  cout << "  A: " << (cpu.accumulator_ & 0x80 ? '1' : '0')
       << (cpu.accumulator_ & 0x40 ? '1' : '0')
       << (cpu.accumulator_ & 0x20 ? '1' : '0')
       << (cpu.accumulator_ & 0x10 ? '1' : '0')
       << (cpu.accumulator_ & 0x08 ? '1' : '0')
       << (cpu.accumulator_ & 0x04 ? '1' : '0')
       << (cpu.accumulator_ & 0x02 ? '1' : '0')
       << (cpu.accumulator_ & 0x01 ? '1' : '0')
       << "  (" << std::hex << setfill('0') << setw(2) << cpu.accumulator_ << ")";

  cout << "  X: " << (cpu.x_reg_ & 0x80 ? '1' : '0')
       << (cpu.x_reg_ & 0x40 ? '1' : '0')
       << (cpu.x_reg_ & 0x20 ? '1' : '0')
       << (cpu.x_reg_ & 0x10 ? '1' : '0')
       << (cpu.x_reg_ & 0x08 ? '1' : '0')
       << (cpu.x_reg_ & 0x04 ? '1' : '0')
       << (cpu.x_reg_ & 0x02 ? '1' : '0')
       << (cpu.x_reg_ & 0x01 ? '1' : '0')
       << "  (" << std::hex << setfill('0') << setw(2) << cpu.x_reg_ << ")";

  cout << "   Y: " << (cpu.y_reg_ & 0x80 ? '1' : '0')
       << (cpu.y_reg_ & 0x40 ? '1' : '0')
       << (cpu.y_reg_ & 0x20 ? '1' : '0')
       << (cpu.y_reg_ & 0x10 ? '1' : '0')
       << (cpu.y_reg_ & 0x08 ? '1' : '0')
       << (cpu.y_reg_ & 0x04 ? '1' : '0')
       << (cpu.y_reg_ & 0x02 ? '1' : '0')
       << (cpu.y_reg_ & 0x01 ? '1' : '0')
       << "  (" << std::hex << setfill('0') << setw(2) << cpu.y_reg_ << ")";

  cout << "  CLK: " << dec << setfill('0') << setw(8) << clk;

  cout << endl;
}

void dump_instruction(const Cpu &cpu,
                      const OpCode &instruction) {
  using namespace std;
  cout << "PC: " << hex << setfill('0') << setw(4) << cpu.pc_;
  cout << "  " << instruction.name;
  cout << setfill(' ') << setw(7);
  switch (instruction.addressing_mode) {
    case OpCode::Accumulator: cout << "Acc";
      break;
    case OpCode::Absolute: cout << "Abs";
      break;
    case OpCode::AbsoluteIndexedX: cout << "Abs,X";
      break;
    case OpCode::AbsoluteIndexedY: cout << "Abs,Y";
      break;
    case OpCode::Immediate: cout << "Imm";
      break;
    case OpCode::Implied: cout << "";
      break;
    case OpCode::Indirect: cout << "Ind";
      break;
    case OpCode::IndirectIndexedX: cout << "Ind,X";
      break;
    case OpCode::IndirectIndexedY: cout << "Ind,Y";
      break;
    case OpCode::Relative: cout << "Rel";
      break;
    case OpCode::ZeroPage: cout << "Zpg";
      break;
    case OpCode::ZeroPageIndexedX: cout << "Zpg,X";
      break;
    case OpCode::ZeroPageIndexedY: cout << "Zpg,Y";
      break;
    default: cout << "ERR";
      break;
  }
  cout << "    | ";
}

int main() {
  using namespace std;

  // Load bin file
  ifstream f("data/6502_functional_test.bin", ios::binary);
  if (!f.is_open()) {
    cerr << "File read failed" << endl;
    return 0;
  }

  auto memory = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
  f.close();

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
    dump_instruction(cpu, iter->second);
    cpu.pc_++;
    iter->second.operation(cpu, memory, clk);
    dump(cpu, clk);

    if (start_pc == cpu.pc_) {
      cout << "Stuck in a loop at PC: 0x" << hex << setw(4) << cpu.pc_ << endl;
      break;
    }
  }

  return EXIT_SUCCESS;
}
