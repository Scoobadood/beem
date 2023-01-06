#include "../include/m6502.h"
#include "cycle_handler.h"
#include "spdlog/spdlog.h"

const uint64_t PIN_DATA_MASK = 0x000000ff;
const uint64_t PIN_ADDR_MASK = 0x00ffff00;
const uint64_t PIN_SYNC = 0x01000000;
const uint64_t PIN_RST = 0x02000000;
const uint64_t PIN_RD_NOT_WR = 0x04000000;

M6502::M6502() {
  stack_pointer_ = 0;
  accumulator_ = 0;
  x_reg_ = 0;
  y_reg_ = 0;
  pc_ = 0;
  ir_ = 0;
  reset_cycle_ = 0;
  reset_in_process_ = false;
  temp_addr_ = 0;
  flags_ = 0;
}

bool M6502::maybe_handle_reset(uint64_t &pins) {
  // If reset low, all else is random, and we're done.
  if (!(pins & PIN_RST)) {
    reset_in_process_ = true;
    reset_cycle_ = 0;
    return true;
  }

  if (!reset_in_process_) {
    return false;
  }
  ++reset_cycle_;
  switch (reset_cycle_) {
    case 1:stack_pointer_ = 0;
      break;

    case 2:set_address(pins, 0x100 + stack_pointer_);
      break;

    case 3:set_address(pins, 0x100 + stack_pointer_ - 1);
      break;

    case 4:set_address(pins, 0x100 + stack_pointer_ - 2);
      break;

    case 5:stack_pointer_ = 0xfd;
      set_address(pins, 0xfffc);
      break;

    case 6:temp_addr_ = get_data(pins);
      set_address(pins, 0xfffd);
      break;

    case 7:reset_in_process_ = false;
      reset_cycle_ = 0;
      pc_ = (get_data(pins) << 8) | temp_addr_;
      set_address(pins, pc_);
      set_SYNC(pins);
      break;
  }
  set_RW(pins);
  return true;
}

void M6502::maybe_handle_sync(uint64_t &pins) {
  if (pins & PIN_SYNC) {
    // load IR register with opcode from data bus, and make room
    // for the 4 bit cycle counter (we only need three but this makes opcodes easier to see)
    // for humans.
    ir_ = get_data(pins) << 4;
    // switch off the SYNC pin
    pins &= ~PIN_SYNC;
    pc_++;
  }
}

void M6502::do_cycle(uint64_t &pins) {
  auto h = cycle_handler(ir_);
  if (h == nullptr) {
    auto msg = fmt::format("No cycle handler for opcode 0x{:02x} cycle {} at PC: 0x{:04x}", ir_ >> 4, ir_ & 0xf, pc_);
    spdlog::critical(msg);
    throw std::runtime_error(msg);
  }
  ir_++;
  h(this, pins);
}

uint64_t M6502::tick(uint64_t pins) {
  if (maybe_handle_reset(pins)) {
    return pins;
  }

  maybe_handle_sync(pins);

  set_RW(pins);

  do_cycle(pins);
  return pins;
}

uint16_t get_address(uint64_t pins) {
  return (pins & PIN_ADDR_MASK) >> 8;
}

void set_address(uint64_t &pins, uint16_t address) {
  pins = (pins & ~PIN_ADDR_MASK) | ((address & 0xffff) << 8);
}

uint8_t get_data(uint64_t pins) {
  return (pins & PIN_DATA_MASK);
}

void set_data(uint64_t &pins, uint8_t data) {
  pins = (pins & ~PIN_DATA_MASK) | (data & 0xff);
}

void set_RST(uint64_t &pins) {
  pins |= PIN_RST;
}

bool tst_RW(uint64_t pins) {
  return (pins & PIN_RD_NOT_WR);
}

void set_RW(uint64_t &pins) {
  pins |= PIN_RD_NOT_WR;
}

void clr_RW(uint64_t &pins) {
  pins &= ~PIN_RD_NOT_WR;
}

bool tst_SYNC(uint64_t pins) {
  return (pins & PIN_SYNC);
}

void set_SYNC(uint64_t &pins) {
  pins |= PIN_SYNC;
}