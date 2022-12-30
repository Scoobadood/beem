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
  ifstream f("data/os120", ios::binary);
  if (!f.is_open()) {
    cerr << "File read failed" << endl;
    return 0;
  }

  auto rom = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
  f.close();
  auto memory = vector<uint8_t>(65535, 0);
  memory.insert(memory.begin() + 0xc000, rom.begin(), rom.end());

  uint32_t buffsize = 20;
  std::vector<std::string> history;
  history.resize(buffsize, "");
  uint32_t next = 0;

  // Set PC
  Cpu cpu;
  cpu.stack_pointer_ = 0xff;
  cpu.pc_ = memory[0xfffc] + memory[0xfffd] * 256;
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
    cout << str1 << str2;
  }
}
