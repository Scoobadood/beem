//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_INCLUDE_SOUND_76489_H_
#define M6502_INCLUDE_SOUND_76489_H_

#include <vector>
class SoundChip {
 public:
  SoundChip();
  void push_byte(uint8_t data);
  void enable();
  void disable();
  inline bool is_enabled() const {
    return is_enabled_;
  }
 private:
  void try_handle_freq_2(uint8_t data);
  void make_sound(uint8_t channel) const;
  bool is_enabled_;
  uint32_t frequency_[3];
  bool playing_[3];
  uint8_t volumes_[3];
  uint8_t noise_volume_;
  uint8_t noise_control_;
  bool noise_playing_;
  uint8_t current_channel_;
  uint8_t partial_frequency_;
  bool expecting_freq_2_;
};

#endif //M6502_INCLUDE_SOUND_76489_H_
