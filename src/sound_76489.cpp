//
// Created by Dave Durbin on 2/1/2023.
//

#include "sound_76489.h"
#include "spdlog/spdlog-inl.h"

SoundChip::SoundChip() {
  for (auto i = 0; i < 3; ++i) {
    frequency_[i] = volumes_[i] = 0;
  }
  noise_volume_ = 0;
  current_channel_ = 0;
  expecting_freq_2_ = false;
  partial_frequency_ = 0;
}
void SoundChip::enable() {
  spdlog::info( "Enabled sound chip");
  is_enabled_ = true;
}

void SoundChip::disable() {
  spdlog::info( "Disabled sound chip");
  is_enabled_ = false;
  playing_[0] = playing_[1] = playing_[2] = noise_playing_ = false;
}

/*
 * R2 R1 R0
 * 0  0  0    Tone 3 frequency
 * 0  0  1    Tone 3 volume
 * 0  1  0    Tone 2 frequency
 * 0  1  1    Tone 2 volume
 * 1  0  0    Tone 1 frequency
 * 1  0  1    Tone 1 volume
 * 1  1  0    Noise control
 * 1  1  1    Noise volume
 */

void SoundChip::try_handle_freq_2(uint8_t data) {
  if (data & 0x80) {
    spdlog::warn("Out of sequence sound packet {:02x}. Expected a FREQ2.", data);
    return;
  }
  frequency_[current_channel_] = partial_frequency_ | ((data & 0x3f) << 4);
  expecting_freq_2_ = false;
  make_sound(current_channel_);
}

void SoundChip::push_byte(uint8_t data) {
  if (expecting_freq_2_) {
    try_handle_freq_2(data);
    return;
  }

  if (!(data & 0x80)) {
    spdlog::warn("Out of sequence sound packet {:02x}. Looks like a FREQ2 but not expecting it.", data);
    return;
  }

  uint8_t command = (data & 0x7f) >> 4;
  if (command == 0 || command == 2 || command == 4) {
    current_channel_ = 2 - (command >> 1);
    expecting_freq_2_ = true;
    partial_frequency_ = data & 0x0f;
    return;
  }

  if (command == 1 || command == 3 || command == 5) {
    auto channel = (command == 1)
                   ? 2
                   : (command == 3)
                     ? 1
                     : 0;
    volumes_[channel] = 15 - (data & 0xf);
    make_sound(channel);
    return;
  }

  if (command == 7) {
    noise_volume_ = 15 - (data & 0xf);
    make_sound(0);
    return;
  }

  // Command is  Noise Control
  noise_control_ = data & 0x7;
  make_sound(0);
}

void SoundChip::make_sound(uint8_t channel) const {
  if (channel == 0) {
    spdlog::info("Playing {} noise at {} frequency on noise channel",
                 (noise_control_ & 0x4) ? "WHITE" : "PERIODIC",
                 std::vector<std::string>{"LOW", "MED", "HIGH", "SINGLE TONE"}[noise_control_ & 0x3]);
    return;
  }

  auto frequency = 4000000 / (32 * frequency_[channel]);
  spdlog::info("Playing note at {} Hz on tone channel {}",
               frequency, channel);

}
