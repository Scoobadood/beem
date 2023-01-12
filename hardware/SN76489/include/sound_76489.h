//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_INCLUDE_SOUND_76489_H_
#define M6502_INCLUDE_SOUND_76489_H_

#include <vector>
#include "6522_via.h"
#include "data_connectors.h"

class SN76489 {
 public:
  SN76489();

  void tick();

  [[nodiscard]] inline data_subscriber_8_bit_ptr we_src() const { return we_src_; }
  [[nodiscard]] inline data_subscriber_8_bit_ptr data_src() const { return data_src_; }

 private:
  void maybe_process_new_data();
  void handle_we(uint8_t we);
  void handle_command(uint8_t command);
  void play_sound() const;

  data_subscriber_8_bit_ptr we_src_;
  data_subscriber_8_bit_ptr data_src_;

  uint8_t latched_reg_;
  uint16_t frequency_[3];
  uint8_t volumes_[3];
  uint8_t noise_volume_;
  uint8_t noise_control_;

  uint8_t we_cycle_count_;
  bool data_expected_;
  bool we_is_low_;
};

#endif //M6502_INCLUDE_SOUND_76489_H_
