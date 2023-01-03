#include "cpu.h"
#include "opcodes.h"

#include <sstream>
#include <functional>
#include <iomanip>
#include <iostream>

Cpu::Cpu(bool should_log) {
  should_log_ = should_log;
}

std::string Cpu::to_string() const {
  using namespace std;

  stringstream oss;

  oss << "PC: " << hex << setfill('0') << setw(4) << pc_;
  oss << "  SP: " << hex << setfill('0') << setw(2) << stack_pointer_;
  oss << "  ST: "
      << (minus() ? 'N' : 'n')
      << (is_overflow() ? 'V' : 'v')
      << '_'
      << '1'
      << (is_decimal() ? 'D' : 'd')
      << (is_interrupt() ? 'I' : 'i')
      << (zero() ? 'Z' : 'z')
      << (carry() ? 'C' : 'c');

  oss << "  A: "
      << (accumulator_ & 0x80 ? '1' : '0')
      << (accumulator_ & 0x40 ? '1' : '0')
      << (accumulator_ & 0x20 ? '1' : '0')
      << (accumulator_ & 0x10 ? '1' : '0')
      << (accumulator_ & 0x08 ? '1' : '0')
      << (accumulator_ & 0x04 ? '1' : '0')
      << (accumulator_ & 0x02 ? '1' : '0')
      << (accumulator_ & 0x01 ? '1' : '0')
      << "  (" << std::hex << setfill('0') << setw(2)
      << accumulator_ << ")";

  oss << "  X: "
      << (x_reg_ & 0x80 ? '1' : '0')
      << (x_reg_ & 0x40 ? '1' : '0')
      << (x_reg_ & 0x20 ? '1' : '0')
      << (x_reg_ & 0x10 ? '1' : '0')
      << (x_reg_ & 0x08 ? '1' : '0')
      << (x_reg_ & 0x04 ? '1' : '0')
      << (x_reg_ & 0x02 ? '1' : '0')
      << (x_reg_ & 0x01 ? '1' : '0')
      << "  (" << std::hex << setfill('0') << setw(2)
      << x_reg_ << ")";

  oss << "   Y: "
      << (y_reg_ & 0x80 ? '1' : '0')
      << (y_reg_ & 0x40 ? '1' : '0')
      << (y_reg_ & 0x20 ? '1' : '0')
      << (y_reg_ & 0x10 ? '1' : '0')
      << (y_reg_ & 0x08 ? '1' : '0')
      << (y_reg_ & 0x04 ? '1' : '0')
      << (y_reg_ & 0x02 ? '1' : '0')
      << (y_reg_ & 0x01 ? '1' : '0')
      << "  (" << std::hex << setfill('0') << setw(2)
      << y_reg_ << ")";
  return oss.str();
}

void Cpu::tick(Memory *memory, uint64_t clock) {
  if (clock != next_clock_) {
    return;
  }
  // Read next instruction and execute it
  auto ins = memory->at(pc_);
  auto iter = codes.find(ins);
  if (iter == codes.end()) {
    auto msg = fmt::format("Unrecognised opcode 0x{:02x} at PC: 0x", ins, pc_);
    spdlog::error(msg);
    throw std::runtime_error(msg);
  }

  auto start_pc = pc_;
  if (start_pc == 0xdeb1) {
    spdlog::warn("Called print message. Y: {}, msg is at 0x{:02x}{:02x}",
                 y_reg_ & 0xff,
                 memory->at(0xfe),
                 memory->at(0xfd));
  }
  pc_++;
  iter->second.operation(*this, *memory, next_clock_);

  auto msg =
      fmt::format("PC: 0x{:04x} {} {} CLK: {}", start_pc, iter->second.to_string(), to_string(), std::to_string(clock));
  history_.at(next_) = msg;
  next_ = (next_ + 1) % history_buffsize_;
  if (should_log_) {
    std::cout << msg << std::endl;
  }

  if (pc_ == start_pc) {
    auto msg = fmt::format("Hung at PC: 0x{:04x}", start_pc);
    spdlog::error(msg);

    for (auto i = 0; i < history_buffsize_; ++i) {
      spdlog::info(history_[next_]);
      next_ = (next_ + 1) % history_buffsize_;
    }
    spdlog::default_logger()->flush();
    throw std::runtime_error(msg);
  }
}
