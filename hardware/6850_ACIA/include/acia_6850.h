//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef BEEB_HARDWARE_ACIA_6850_H_
#define BEEB_HARDWARE_ACIA_6850_H_

#include "bus.h"

#include <cstdint>

class Acia {
 public:
  Acia();

  void tick(Bus &bus);

 private:
  void master_reset();
  void write_ctl(uint8_t data, Bus & bus);
  void mmio_write(uint16_t addr, Bus &bus);

  /* Control register */
  uint8_t clk_divisor_{};
  uint8_t stop_bits_{};
  uint8_t word_length_{};
  uint8_t parity_{}; // 0 = none, 1 == odd, 2 == even
  bool tx_int_enabled_{};
  bool rx_int_enabled_{};
  bool rts_default_low_{};
};

#endif // BEEB_HARDWARE_ACIA_6850_H_
