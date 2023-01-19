//
// Created by Dave Durbin on 19/1/2023.
//

#include "clock.h"

void Clock::tick() {
  clks_prev_ = clks_;

  clks_ ^= CLK_16_MHZ;
  ticks_ = (ticks_ + 1) % 16;
  if (ticks_ % 2 == 1) clks_ ^= CLK_8_MHZ;
  if (ticks_ % 4 == 1) clks_ ^= CLK_4_MHZ;
  if (ticks_ % 8 == 1) clks_ ^= CLK_2_MHZ;
  if (ticks_ % 16 == 1) clks_ ^= CLK_1_MHZ;
}