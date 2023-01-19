//
// Created by Dave Durbin on 19/1/2023.
//

#ifndef CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_
#define CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_

#include <cstdint>

const uint8_t CLK_1_MHZ = (0x01 << 0);
const uint8_t CLK_2_MHZ = (0x01 << 1);
const uint8_t CLK_4_MHZ = (0x01 << 2);
const uint8_t CLK_8_MHZ = (0x01 << 3);
const uint8_t CLK_16_MHZ = (0x01 << 4);

class Clock {
 public:
  Clock() {
    clks_ = 0x1f;
    clks_prev_ = 0x00;
    ticks_ = 0;
  }

  void tick();
  [[nodiscard]] inline bool is_high(uint8_t clk) const { return clks_ & clk; }
  [[nodiscard]] inline bool is_low(uint8_t clk) const { return (clks_ & clk) == 0; }
  [[nodiscard]] inline bool went_high(uint8_t clk) const { return (clks_ & clk) && ((clks_prev_ & clk) == 0); }
  [[nodiscard]] inline bool went_low(uint8_t clk) const { return (clks_ & clk) == 0 && (clks_prev_ & clk); }
  [[nodiscard]] inline bool changed(uint8_t clk) const { return (clks_ & clk) != (clks_prev_ & clk); }

 private:
  uint8_t ticks_;
  uint8_t clks_;
  uint8_t clks_prev_;
};
#endif //CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_
