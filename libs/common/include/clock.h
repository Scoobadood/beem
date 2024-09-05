//
// Created by Dave Durbin on 19/1/2023.
//

#ifndef CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_
#define CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_

#include <cstdint>

const uint8_t CLK_1_MHZ = (0x01 << 0);
const uint8_t CLK_E_2_MHZ = (0x01 << 1);
const uint8_t CLK_2_MHZ = (0x01 << 2);
const uint8_t CLK_4_MHZ = (0x01 << 3);
const uint8_t CLK_8_MHZ = (0x01 << 4);
const uint8_t CLK_16_MHZ = (0x01 << 5);
const uint8_t CLK_ALL = (CLK_1_MHZ | CLK_2_MHZ | CLK_E_2_MHZ | CLK_4_MHZ | CLK_8_MHZ | CLK_16_MHZ);

class Clock {
public:
  Clock();

  void tick();

  void begin_time_stretch();

  [[nodiscard]] inline bool is_stretched() const { return (stretch_time_ > 0); }

  [[nodiscard]] inline bool is_high(uint8_t clk) const { return clks_ & clk; }

  [[nodiscard]] inline bool is_low(uint8_t clk) const { return (clks_ & clk) == 0; }

  [[nodiscard]] inline bool went_high(uint8_t clk) const { return (clks_ & clk) && ((clks_prev_ & clk) == 0); }

  [[nodiscard]] inline bool went_low(uint8_t clk) const { return (clks_ & clk) == 0 && (clks_prev_ & clk); }

  [[nodiscard]] inline bool changed(uint8_t clk) const { return (clks_ & clk) != (clks_prev_ & clk); }

private:
  uint8_t ticks_;
  uint8_t clks_;
  uint8_t clks_prev_;
  uint8_t stretch_time_;
  const bool *stretch_seq_;
};

#endif //CHIPS_M6502_HARDWARE_CLOCKS_CLOCK_H_
