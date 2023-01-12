/**
 * Note that the way in which the AUG documents the chip and it's connection to SystemVIA PA0-PA7
 * varies from the chip spec itself http://www.vgmpf.com/Wiki/images/7/78/SN76489AN_-_Manual.pdf
 * in the sense that in the spec, bit 0 is high nd 7 is low while in AUG, bit 0 is low.
 * The AUG also reverses the tone registers (3==1, 1==3)
 *
 */
#include "sound_76489.h"

#include <spdlog/spdlog-inl.h>

SN76489::SN76489() //
    : we_src_{nullptr} //
    , data_src_{nullptr} //
    , latched_reg_{0} //
    , frequency_{0, 0, 0} //
    , volumes_{0, 0, 0} //
    , noise_volume_{0} //
    , noise_control_{0} //
    , we_cycle_count_{0} //
    , data_expected_{false} //
    , we_is_low_{false} //
{
  we_src_ = std::make_shared<data_subscriber_8_bit>(0x01);
  data_src_ = std::make_shared<data_subscriber_8_bit>(0xff);
}

void SN76489::play_sound() const {
  const std::vector<std::string> noise_freq{"LOW", "MED", "HIGH", "SINGLE TONE"};

  if (noise_volume_ != 0) {
    spdlog::info("Playing {} noise at {} frequency, volume {} on noise channel",
                 (noise_control_ & 0x4) ? "WHITE" : "PERIODIC",
                 noise_freq[noise_control_ & 0x3], noise_volume_);
  }

  for (auto channel = 0; channel < 3; ++channel) {
    if (volumes_[channel] == 0) continue;
    auto frequency = 4000000 / (32 * frequency_[channel]);
    spdlog::info("Playing {}Hz note at volume {} on channel {}", frequency, volumes_[channel], channel);
  }
}

void SN76489::handle_command(uint8_t command) {
  if (command & 0x80) {
    auto reg = (command & 0x70) >> 4;
    auto data = (command & 0x0f);
    switch (reg) {
      case 0:frequency_[2] = ((frequency_[2] & 0xfff0) | data);
        break;
      case 2:frequency_[1] = ((frequency_[1] & 0xfff0) | data);
        break;
      case 4:frequency_[0] = ((frequency_[0] & 0xfff0) | data);
        break;

      case 1:volumes_[2] = (~data) & 0xf;
        break;
      case 3:volumes_[1] = (~data) & 0xf;
        break;
      case 5:volumes_[0] = (~data) & 0xf;
        break;

      case 6:noise_control_ = (data & 0x07);
        break;
      case 7:noise_volume_ = (~data) & 0xf;
        break;

      default:spdlog::critical("SN76489: Invalid register ({:02x}) specified", reg);
    }
    latched_reg_ = reg;
  } else {
    auto data = (command & 0x3f);

    // Direct to the high bits of the latched register
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
                         command);
    }
  }
  play_sound();
}

void SN76489::maybe_process_new_data() {
  if (!data_expected_) return;
  if (!data_src_->data_changed()) return;

  handle_command(data_src_->data());
  data_expected_ = false;
}

void SN76489::handle_we(uint8_t we) {
  // State unchanged high
  if (we & !we_is_low_) return;

  // State unchanged lo
  if (~we & we_is_low_) return;

  // State changed from high to low
  if (~we & !we_is_low_) {
    spdlog::info("SN76489: WE set high");
    we_cycle_count_ = 0;
    data_expected_ = false;
    we_is_low_ = true;
    return;
  }

  // Implicitly state changed from low to high
  spdlog::info("SN76489: WE pulled low");
  if (we_cycle_count_ > 8) data_expected_ = true;
  we_is_low_ = false;
}

void SN76489::tick() {
  if (data_expected_) {
    maybe_process_new_data();
  }
  if (we_src_->data_changed()) {
    handle_we(we_src_->data());
  }
  if (!we_is_low_) {
    // Count low cycles
    ++we_cycle_count_;
  }
}
