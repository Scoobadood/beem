//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_INCLUDE_SOUND_76489_H_
#define M6502_INCLUDE_SOUND_76489_H_

#include <vector>
#include "6522_via.h"

using SN76489_data_source= std::function<uint8_t()>;

class SN76489 {
 public:
  SN76489();

  void tick();

  void set_data_source(SN76489_data_source src);

  void enable(bool enable);

 private:
  void make_sound() const;

  SN76489_data_source src_;

  bool is_enabled_;
  uint8_t latched_reg_;
  uint16_t frequency_[3];
  uint8_t volumes_[3];
  uint8_t noise_volume_;
  uint8_t noise_control_;
};

#endif //M6502_INCLUDE_SOUND_76489_H_
