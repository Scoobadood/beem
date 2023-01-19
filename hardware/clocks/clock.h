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
    clks_ = clks_prev_ = 0;
    ticks_ = 0;
  }

  void tick();
  inline bool is_high(uint8_t clk) { return clks_ & clk;}
  inline bool is_low(uint8_t clk) { return (clks_ & clk) == 0;}
  inline bool went_high(uint8_t clk) { return (clks_ & clk) && ( (clks_prev_ & clk) == 0);}
  inline bool went_low(uint8_t clk){ return (clks_ & clk)==0 && ( clks_prev_ & clk);}
  inline bool changed(uint8_t clk) { return (clks_ & clk) != (clks_prev_ & clk);}

 private:
  uint8_t ticks_;
  uint8_t clks_;
  uint8_t clks_prev_;
};

void Clock::tick() {
  clks_prev_ = clks_;

  clks_ ^= CLK_16_MHZ;
  ticks_ = (ticks_ + 1) % 16;
  if (ticks_ % 2 == 1) clks_ ^= CLK_8_MHZ;
  if (ticks_ % 4 == 1) clks_ ^= CLK_4_MHZ;
  if (ticks_ % 8 == 1) clks_ ^= CLK_2_MHZ;
  if (ticks_ % 16 == 1) clks_ ^= CLK_1_MHZ;
}

#endif //CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_
