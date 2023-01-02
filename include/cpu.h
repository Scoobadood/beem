//
// Created by Dave Durbin on 30/11/2022.
//

#ifndef CPU_CPU_H_
#define CPU_CPU_H_

#include <vector>
#include <bitset>
#include "memory.h"

const uint8_t SR_NEG = 7;
const uint8_t SR_OVF = 6;
const uint8_t SR_DEC = 3;
const uint8_t SR_INT = 2;
const uint8_t SR_ZER = 1;
const uint8_t SR_CRY = 0;

struct Cpu {
  Cpu(bool should_log = false);

  [[nodiscard]] inline bool minus() const {
    return status_.test(SR_NEG);
  }
  [[nodiscard]] inline bool plus() const {
    return !status_.test(SR_NEG);
  }
  [[nodiscard]] inline bool is_overflow() const {
    return status_.test(SR_OVF);
  }
  [[nodiscard]] inline bool is_decimal() const {
    return status_.test(SR_DEC);
  }
  [[nodiscard]] inline bool is_interrupt() const {
    return status_.test(SR_INT);
  }
  [[nodiscard]] inline bool zero() const {
    return status_.test(SR_ZER);
  }
  [[nodiscard]] inline bool not_zero() const {
    return !status_.test(SR_ZER);
  }
  [[nodiscard]] inline bool carry() const {
    return status_.test(SR_CRY);
  }
  [[nodiscard]] inline bool carry_clear() const {
    return !status_.test(SR_CRY);
  }
  inline void set_neg() {
    status_.set(SR_NEG, true);
  }
  inline void set_overflow() {
    status_.set(SR_OVF, true);
  }
  inline void set_decimal() {
    status_.set(SR_DEC, true);
  }
  inline void set_interrupt() {
    status_.set(SR_INT, true);
  }
  inline void set_zero() {
    status_.set(SR_ZER, true);
  }
  inline void set_carry() {
    status_.set(SR_CRY, true);
  }
  inline void clear_neg() {
    status_.set(SR_NEG, false);
  }
  inline void clear_overflow() {
    status_.set(SR_OVF, false);
  }
  inline void clear_decimal() {
    status_.set(SR_DEC, false);
  }
  inline void clear_interrupt() {
    status_.set(SR_INT, false);
  }
  inline void clear_zero() {
    status_.set(SR_ZER, false);
  }
  inline void clear_carry() {
    status_.set(SR_CRY, false);
  }

  std::string to_string() const;

  void tick(Memory *memory, uint64_t clock_);

  uint32_t pc_;
  std::bitset<8> status_;
  uint16_t stack_pointer_;
  uint16_t accumulator_;
  uint16_t x_reg_;
  uint16_t y_reg_;
  std::vector<uint8_t> stack_;

  uint64_t next_clock_;
  bool should_log_;

};
#endif //CPU_CPU_H_
