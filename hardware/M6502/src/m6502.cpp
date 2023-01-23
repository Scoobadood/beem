#include "m6502.h"
#include "cycle_handler.h"

#include <spdlog/spdlog-inl.h>

M6502::M6502() //
    : brk_flags_{0} //
    , pc_{0} //
    , flags_{0} //
    , stack_pointer_{0} //
    , accumulator_{0} //
    , x_reg_{0} //
    , y_reg_{0} //
    , temp_addr_{0} //
    , ir_{0} //
    , reset_in_process_{false} //
    , reset_cycle_{0} //
    , next_history_buffer_entry_{0} //
{
  opcode_history_buffer_.resize(history_size_, 0xff);
  pc_history_buffer_.resize(history_size_, 0xffff);
}

bool M6502::maybe_handle_reset(const std::shared_ptr<Bus> &bus) {
  // If reset low, all else is random, and we're done.
  if (!bus->tst_RST()) {
    reset_in_process_ = true;
    reset_cycle_ = 0;
    return true;
  }

  // RST is high again. Either continue with reset sequence of return false if we're not mid reset.
  if (!reset_in_process_) {
    return false;
  }
  ++reset_cycle_;
  switch (reset_cycle_) {
    case 1:stack_pointer_ = 0;
      break;

    case 2:bus->set_address(0x100 + stack_pointer_);
      break;

    case 3:bus->set_address(0x100 + stack_pointer_ - 1);
      break;

    case 4:bus->set_address(0x100 + stack_pointer_ - 2);
      break;

    case 5:stack_pointer_ = 0xfd;
      bus->set_address(0xfffc);
      break;

    case 6:temp_addr_ = bus->get_data();
      bus->set_address(0xfffd);
      break;

    case 7:reset_in_process_ = false;
      reset_cycle_ = 0;
      pc_ = (bus->get_data() << 8) | temp_addr_;
      bus->set_address(pc_);
      bus->set_SYNC();
      break;
  }
  bus->set_RW();
  return true;
}

void M6502::maybe_handle_sync(const std::shared_ptr<Bus> &bus) {
  if (bus->tst_SYNC()) {
    // load IR register with opcode from data bus, and make room
    // for the 4 bit cycle counter (we only need three but this makes opcodes easier to see)
    // for humans.
    opcode_history_buffer_[next_history_buffer_entry_] = bus->get_data();
    pc_history_buffer_[next_history_buffer_entry_] = bus->get_address();
    next_history_buffer_entry_ = (next_history_buffer_entry_ + 1) % history_size_;

    ir_ = bus->get_data() << 4;
    // switch off the SYNC pin
    bus->clr_SYNC();
    pc_++;
  }
}

void M6502::do_cycle(const std::shared_ptr<Bus> &bus) {
  auto handler = cycle_handler(ir_);
  if (handler == nullptr) {
    auto msg = fmt::format("No cycle handler for opcode 0x{:02x} cycle {} at_bus PC: 0x{:04x}", ir_ >> 4, ir_ & 0xf, pc_);
    spdlog::critical(msg);
    for (auto i = 0; i < history_size_; ++i) {
      auto nn = (next_history_buffer_entry_ + i) % history_size_;
      spdlog::critical("0x{:04x}  {:02x}",
                       pc_history_buffer_[nn],
                       opcode_history_buffer_[nn]
      );
    }
    spdlog::default_logger()->flush();
    throw std::runtime_error(msg);
  }
  ir_++;
  handler(this, *bus);
}

void M6502::tick(const std::shared_ptr<Bus> &bus) {
  if (maybe_handle_reset(bus)) {
    return;
  }

  maybe_handle_sync(bus);

  bus->set_RW();

  do_cycle(bus);
}