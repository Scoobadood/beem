//
// Created by Dave Durbin on 30/11/2022.
//

#ifndef BEEB_CPU_6502_H
#define BEEB_CPU_6502_H

#include <vector>
#include <bitset>

const uint8_t HIGH = 1;
const uint8_t LOW = 0;

// Status Register flags
const uint8_t FLAG_C = 1 << 0;
const uint8_t FLAG_Z = 1 << 1;
const uint8_t FLAG_I = 1 << 2;
const uint8_t FLAG_D = 1 << 3;
const uint8_t FLAG_B = 1 << 4;
const uint8_t FLAG_X = 1 << 4;
const uint8_t FLAG_V = 1 << 6;
const uint8_t FLAG_N = 1 << 7;

class M6502 {
 public:
  M6502();

  uint64_t tick(uint64_t pins);

  /* General registers */
  inline void set_x(uint8_t x) {
    x_reg_ = x;
  }
  inline uint8_t x() const {
    return x_reg_;
  }
  inline void dec_x() {
    x_reg_--;
  }
  inline void set_y(uint8_t y) {
    y_reg_ = y;
  }
  inline uint8_t y() const {
    return y_reg_;
  }
  inline void dec_y() {
    y_reg_--;
  }
  inline void set_a(uint8_t a) {
    accumulator_ = a;
  }
  inline uint8_t a() const {
    return accumulator_;
  }

  inline uint16_t pc() const {
    return pc_;
  }
  inline uint16_t inc_pc() {
    return pc_++;
  }
  inline void set_pc(uint16_t pc) {
    pc_ = pc;
  }
  inline uint8_t sp() const {
    return stack_pointer_;
  }
  inline void set_sp(uint8_t sp) {
    stack_pointer_ = sp;
  }

  /* Flag manipulation */
  inline void setC() {
    flags_ |= FLAG_C;
  }
  inline void clrC() {
    flags_ &= ~FLAG_C;
  }
  inline bool tstC() const {
    return (flags_ & FLAG_C);
  }
  inline void setD() {
    flags_ |= FLAG_D;
  }
  inline void clrD() {
    flags_ &= ~FLAG_D;
  }
  inline bool tstD() const {
    return (flags_ & FLAG_D);
  }
  inline void setV() {
    flags_ |= FLAG_V;
  }
  inline void clrV() {
    flags_ &= ~FLAG_V;
  }
  inline void setZ() {
    flags_ |= FLAG_Z;
  }
  inline void clrZ() {
    flags_ &= ~FLAG_Z;
  }
  inline bool tstZ() {
    return (flags_ &= FLAG_Z);
  }
  inline void setN() {
    flags_ |= FLAG_N;
  }
  inline void clrN() {
    flags_ &= ~FLAG_N;
  }
  inline bool tstN() {
    return (flags_ &= FLAG_N);
  }
  inline void setNZ(uint8_t value) {
    flags_ = (value) ? (flags_ & ~FLAG_Z) : (flags_ | FLAG_Z);
    flags_ = (value & 0x80) ? (flags_ | FLAG_N) : (flags_ &= ~FLAG_N);
  }

  /* Temporary storage buffers */
  inline void set_temp_addr_low(uint8_t low) {
    temp_addr_ = low;
  }
  inline void set_temp_addr(uint16_t addr) {
    temp_addr_ = addr;
  }
  inline uint16_t temp_address() const {
    return temp_addr_;
  }

 private:
  uint16_t pc_;
  uint8_t flags_;
  uint16_t stack_pointer_;
  uint8_t accumulator_;
  uint8_t x_reg_;
  uint8_t y_reg_;

  // Temp address data
  uint16_t temp_addr_;
  // Instruction register with 3 bits of phasing
  uint16_t ir_;
  bool reset_in_process_;
  uint8_t reset_cycle_;

  // Internal utility
  bool maybe_handle_reset(uint64_t &pins);
  void maybe_handle_sync(uint64_t &pins);
  void do_cycle(uint64_t &pins);
};

uint16_t get_address(uint64_t pins);
void set_address(uint64_t &pins, uint16_t address);
uint8_t get_data(uint64_t pins);
void set_data(uint64_t &pins, uint8_t data);
void set_RST(uint64_t &pins);
bool tst_RW(uint64_t pins);
void set_RW(uint64_t &pins);
void clr_RW(uint64_t &pins);
void set_SYNC(uint64_t &pins);

#endif //BEEB_CPU_6502_H
