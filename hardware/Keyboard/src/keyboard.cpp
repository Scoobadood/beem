//
// Created by Dave Durbin on 2/1/2023.
//

#include "keyboard.h"
#include <spdlog/spdlog-inl.h>

Keyboard::Keyboard() //
    : dips_{0} //
    , auto_scan_enabled_{true} //
{
  we_src_ = std::make_shared<data_subscriber_8_bit>(0x04);
  data_src_ = std::make_shared<data_subscriber_8_bit>(0x7f);
  provider_ = std::make_shared<data_provider_8_bit>();
}

void Keyboard::tick() {
  check_we();
  if (data_src_->data_changed()) {
    handle_command(data_src_->data());
  }
  // TODO: If auto_scan_enabled_ poll keyb and generate interrupt
  // Needs a CA1 provider
}

void Keyboard::check_we() {
  if (!we_src_->data_changed()) return;
  auto enable = we_src_->data();
  if (enable) {
    auto_scan_enabled_ = true;
    spdlog::info("Keyboard auto scan enabled");
  } else {
    auto_scan_enabled_ = false;
    spdlog::info("Keyboard auto scan disabled");
  }
}

void Keyboard::handle_command(uint8_t key_code) {
  key_code &= 0x7f;
  auto scan_result = key_code;
  if (key_code >= KEY_DIP_7 && key_code <= KEY_DIP_0) {
    auto s = 9 - key_code;
    auto tst = 0x01 << s;
    if (dips_ & tst) {
      scan_result |= 0x80;
    }
  }
  provider()->provide_data(scan_result);
}