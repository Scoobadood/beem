//
// Created by Dave Durbin on 19/1/2023.
//

#include "clock.h"

const bool in_phase_seq[]{false, false, true};
const bool out_of_phase_seq[]{false, false, true, false, false};

Clock::Clock() //
        : ticks_{0}//
        , clks_{0x00} //
        , clks_prev_{CLK_ALL} //
        , stretch_time_{0} //
        , stretch_seq_{0} //
{}

void Clock::begin_time_stretch() {
  /* In phase, skip 3 cycles */
  if ((is_high(CLK_E_2_MHZ) && is_high(CLK_1_MHZ)) ||
      (is_low(CLK_E_2_MHZ) && is_low(CLK_1_MHZ))) {
    stretch_seq_ = in_phase_seq;
    stretch_time_ = 3;
  } else {
    stretch_seq_ = out_of_phase_seq;
    stretch_time_ = 5;
  }
}

void Clock::tick() {
  clks_prev_ = clks_;

  ticks_ = (ticks_ + 1) % 16;

  clks_ ^= CLK_16_MHZ;
  if (ticks_ % 2 == 0) clks_ ^= CLK_8_MHZ;
  if (ticks_ % 4 == 0) clks_ ^= CLK_4_MHZ;
  if (ticks_ % 8 == 0) clks_ ^= CLK_2_MHZ;
  if (ticks_ % 16 == 0) clks_ ^= CLK_1_MHZ;

  // Varies with time stretching
  if (ticks_ % 8 == 0) {
    bool should_flip = true;
    if (stretch_time_ > 0) {
      should_flip = stretch_seq_[--stretch_time_];
    }
    if (should_flip)
      clks_ ^= CLK_E_2_MHZ;
  }
}