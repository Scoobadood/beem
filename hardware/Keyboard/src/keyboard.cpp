//
// Created by Dave Durbin on 2/1/2023.
//

#include "keyboard.h"
#include <spdlog/spdlog-inl.h>

Keyboard::Keyboard(uint8_t dips) //
    : dips_{dips} //
    , auto_scan_enabled_{true} //
    , caps_lock_led_{false} //
    , shift_lock_led_{false} //
{
  we_src_ = std::make_shared<data_subscriber_8_bit>(0x08);
  data_src_ = std::make_shared<data_subscriber_8_bit>(0x7f);
  provider_ = std::make_shared<data_provider_8_bit>();
  cl_led_src_ = std::make_shared<data_subscriber_8_bit>(0x40);
  sl_led_src_ = std::make_shared<data_subscriber_8_bit>(0x80);
}

void Keyboard::tick() {
  check_leds();
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

void log_state(const std::string &led_name, bool old_state, bool new_state) {
  std::string action = (old_state == new_state) ? "still" : "turned";
  std::string state = new_state ? "on" : "off";
  spdlog::info("Keyboard: {} LED {} {}", led_name, action, state);
}

void Keyboard::check_leds() {
  if (cl_led_src_->data_changed()) {
    auto on = (cl_led_src_->data() == 0x40);
    log_state("CAPS Lock", caps_lock_led_, on);
    caps_lock_led_ = on;
  }
  if (sl_led_src_->data_changed()) {
    auto on = (sl_led_src_->data() == 0x80);
    log_state("SHIFT Lock", shift_lock_led_, on);
    shift_lock_led_ = on;
  }
}

void Keyboard::handle_command(uint8_t key_code) const {
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