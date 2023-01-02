#include "cpu.h"

#include <sstream>
#include <functional>
#include <iomanip>

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

  oss << "  CLK: " << dec << setfill('0') << setw(8);

  oss << endl;
  return oss.str();
}