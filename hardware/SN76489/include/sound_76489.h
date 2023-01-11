//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_INCLUDE_SOUND_76489_H_
#define M6502_INCLUDE_SOUND_76489_H_

#include <vector>
#include "6522_via.h"

class SN76489 {
 public:
  SN76489();

  void tick();

  void operator()(uint8_t data);
  void set_write_enable(uint8_t we);

 private:
  void handle_waiting_command();
  void make_sound() const;

  uint8_t latched_reg_;
  uint16_t frequency_[3];
  uint8_t volumes_[3];
  uint8_t noise_volume_;
  uint8_t noise_control_;


  uint8_t last_we_state_;
  uint8_t we_cycle_count_;
  bool data_expected_;
  bool command_waiting_;
  uint8_t command_;
};

#endif //M6502_INCLUDE_SOUND_76489_H_
