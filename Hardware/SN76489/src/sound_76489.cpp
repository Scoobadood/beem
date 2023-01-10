/**
 * Note that the way in which the AUG documents the chip and it's connection to SystemVIA PA0-PA7
 * varies from the chip spec itself http://www.vgmpf.com/Wiki/images/7/78/SN76489AN_-_Manual.pdf
 * in the sense that the mappings of pins for commands vary.
 * AUG has the high bit indicating if this is a first or subsequent byte, then bits R2 R1 R0 for register select
 * The chip spec has the low it indicating first/subsequent followed by R0 R1 R2 in bit 1:3
 * This file assumes the BBC format.
 *
 */
#include "sound_76489.h"

#include <spdlog/spdlog-inl.h>

SN76489::SN76489() //
    : src_{nullptr} //
    , is_enabled_{false} //
    , latched_reg_{0} //
    , frequency_{0, 0, 0} //
    , volumes_{0, 0, 0} //
    , noise_volume_{0} //
    , noise_control_{0} //
{
  noise_volume_ = 0;
  noise_control_ = 0;
}

void SN76489::set_data_source(SN76489_data_source src) {
  src_ = std::move(src);
}

void SN76489::enable(bool enable) {
  is_enabled_ = enable;
  spdlog::info("{}} sound chip", enable ? "Enbled" : "Disabled");
}

void SN76489::make_sound() const {
  spdlog::info("Playing {} noise at {} frequency, volume {} on noise channel",
               (noise_control_ & 0x4) ? "WHITE" : "PERIODIC",
               std::vector<std::string>{"LOW", "MED", "HIGH", "SINGLE TONE"}[noise_control_ & 0x3], noise_volume_);

  for (auto channel = 0; channel < 3; ++channel) {
    auto frequency = 4000000 / (32 * frequency_[channel]);
    spdlog::info("Playing {}Hz note at volume {} on channel {}", frequency, volumes_[channel], channel);
  }
}

void SN76489::tick() {
  if (is_enabled_ && src_) {
    auto cmd = src_();

    if (cmd & 0x80) {
      auto reg = (cmd & 0x70) >> 4;
      auto data = (cmd & 0x0f);
      switch (reg) {
        case 0:frequency_[2] = ((frequency_[2] & 0xfff0) | data);
          break;
        case 2:frequency_[1] = ((frequency_[1] & 0xfff0) | data);
          break;
        case 4:frequency_[0] = ((frequency_[0] & 0xfff0) | data);
          break;

        case 1:volumes_[2] = data;
          break;
        case 3:volumes_[1] = data;
          break;
        case 5:volumes_[0] = data;
          break;

        case 6:noise_control_ = (data & 0x07);
          break;
        case 7:noise_volume_ = data;
          break;

        default:spdlog::critical("SN76489: Invalid register ({:02x}) specified", reg);
      }
      latched_reg_ = reg;
    } else {
      auto data = (cmd & 0x3f);

      // Direct to the low bits of the latched register
      switch (latched_reg_) {
        case 0:frequency_[2] = ((frequency_[2] & 0x000f) | (data << 4));
          break;
        case 2:frequency_[1] = ((frequency_[1] & 0x000f) | (data << 4));
          break;
        case 4:frequency_[0] = ((frequency_[0] & 0x000f) | (data << 4));
          break;
        default:
          spdlog::critical("SN76489: Unexpected latched reg ({:02x}) for low freq cmd ({:02x})",
                           latched_reg_,
                           cmd);
      }
    }
  }
  make_sound();
}