//
// Created by Dave Durbin on 30/11/2022.
//

#ifndef BEEB_CPU_6502_H
#define BEEB_CPU_6502_H

#include "bus.h"
#include "spdlog/spdlog.h"

#include <vector>
#include <bitset>

// internal BRK flags - what sort of BRK is being services
const uint8_t BRK_IRQ = 1 << 0;  /* IRQ in progress */
const uint8_t BRK_NMI = 1 << 1;  /* NMI in progress */
const uint8_t BRK_RST = 1 << 2;  /* RES in progress */

// Status Register flags
const uint8_t FLAG_N = 1 << 7;
const uint8_t FLAG_V = 1 << 6;
const uint8_t FLAG_X = 1 << 5;
const uint8_t FLAG_B = 1 << 4;
const uint8_t FLAG_D = 1 << 3;
const uint8_t FLAG_I = 1 << 2;
const uint8_t FLAG_Z = 1 << 1;
const uint8_t FLAG_C = 1 << 0;

class M6502 {
public:
  M6502();

  void tick(const std::shared_ptr<Bus> &bus);

  /* General registers */
  inline void setX(uint8_t x) { x_reg_ = x; }

  inline void decX() { x_reg_--; }

  inline void incX() { x_reg_++; }

  inline uint8_t X() const { return x_reg_; }

  inline void setY(uint8_t y) { y_reg_ = y; }

  inline void decY() { y_reg_--; }

  inline void incY() { y_reg_++; }

  inline uint8_t Y() const { return y_reg_; }

  inline void setA(uint8_t a) { accumulator_ = a; }

  inline uint8_t A() const { return accumulator_; }

  inline void setPC(uint16_t pc) { pc_ = pc; }

  inline uint16_t incPC() { return pc_++; }

  inline uint16_t PC() const { return pc_; }

  inline void setSP(uint8_t sp) { stack_pointer_ = sp; }

  /* Return SP and decrement */
  inline uint8_t decSP() { return stack_pointer_--; }

  /* Increment SP and return */
  inline uint8_t incSP() { return ++stack_pointer_; }

  inline uint8_t SP() const { return stack_pointer_; }

  /* Flag manipulation */
  inline void set_flags(uint8_t flags) { flags_ = flags; }

  inline uint8_t flags() const { return flags_; }

  inline void setC() { flags_ |= FLAG_C; }

  inline void clrC() { flags_ &= ~FLAG_C; }

  inline bool tstC() const { return (flags_ & FLAG_C); }

  inline void setD() { flags_ |= FLAG_D; }

  inline void clrD() { flags_ &= ~FLAG_D; }

  inline bool tstD() const { return (flags_ & FLAG_D); }

  inline void setV() { flags_ |= FLAG_V; }

  inline void clrV() { flags_ &= ~FLAG_V; }

  inline bool tstV() const { return (flags_ & FLAG_V); }

  inline void setZ() { flags_ |= FLAG_Z; }

  inline void clrZ() { flags_ &= ~FLAG_Z; }

  inline bool tstZ() const { return (flags_ & FLAG_Z); }

  inline void setN() { flags_ |= FLAG_N; }

  inline void clrN() { flags_ &= ~FLAG_N; }

  inline bool tstN() const { return (flags_ & FLAG_N); }

  inline void setI() { flags_ |= FLAG_I; }

  inline void clrI() { flags_ &= ~FLAG_I; }

  inline bool tstI() const { return (flags_ & FLAG_I); }

  inline void setB() { flags_ |= FLAG_B; }

  inline void clrB() { flags_ &= ~FLAG_B; }

  inline void setNZ(uint8_t value) {
    // Turn off N and Z
    flags_ = flags_ & ~(FLAG_N | FLAG_Z);

    // Turn on Z or N
    flags_ |= ((value & 0xff) ? (value & FLAG_N) : FLAG_Z);
  }

  /* Temporary storage buffers */
  inline void set_temp_addr_high(uint8_t high) { temp_addr_ |= (high << 8); }

  inline void set_temp_addr_low(uint8_t low) { temp_addr_ |= low; }

  inline void set_temp_addr(uint16_t addr) { temp_addr_ = addr; }

  inline uint16_t temp_addr_low() const { return temp_addr_ & 0xff; }

  inline uint16_t temp_addr_high() const { return temp_addr_ & 0xff00; }

  inline uint16_t temp_addr() const { return temp_addr_; }

  // Used by indirect addressing modes when page doesn't wrap
  inline void skip_cycle() { ir_++; };

  inline void raise_irq() {
    interrupt_requested_ = true;
  }

  inline void clear_irq() {
    interrupt_requested_ = false;
  }

  inline void raise_nmi() {
  }

  uint8_t brk_flags_;
private:

  // Internal utility
  bool maybe_handle_reset(const std::shared_ptr<Bus> &bus);


  void maybe_handle_sync(const std::shared_ptr<Bus> &bus);

  void do_cycle(const std::shared_ptr<Bus> &bus);


  uint16_t pc_;
  uint8_t flags_;
  uint16_t stack_pointer_;
  uint8_t accumulator_;
  uint8_t x_reg_;
  uint8_t y_reg_;
  bool interrupt_requested_;

  // Temp address data
  uint16_t temp_addr_;
  // Instruction register with 4 bits of phasing
  uint16_t ir_;
  bool reset_in_process_;
  uint8_t reset_cycle_;

  // DEBUG Tools
  // Last 5 opcodes and PC
  static const uint8_t history_size_ = 50;
  std::vector<uint8_t> opcode_history_buffer_;
  std::vector<uint16_t> pc_history_buffer_;
  uint8_t next_history_buffer_entry_;
};


#endif //BEEB_CPU_6502_H
