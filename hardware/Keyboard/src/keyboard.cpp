//
// Created by Dave Durbin on 2/1/2023.
//

#include "keyboard.h"
#include <spdlog/spdlog-inl.h>

/* Auto scan scans one column at a time, ignoring
 * The first row (DIPS SHIFT and CTRL)
 *                                                                        <-- Master only ->
 *       0x00      0x01  0x02  0x03 0x04 0x05 0x06 0x07 0x08 0x09       0x0a   0x0b   0x0c
 * 0x00  Shift     Ctrl  <------- startup up DIP swicthes ------->
 * 0x10  Q         3     4     5    f4   8    f7   =-   ~^   Left       KP 6   KP 7
 * 0x20  f0        W     E     T    7    I    9    0    £    Down       KP 8   KP 9
 * 0x30  1         2     D     R    6    U    O    P    [{   Up         KP +   KP -   KP Return
 * 0x40  CapsLck   A     X     F    Y    J    K    @    :*   Return     KP /   KP Del KP .
 * 0x50  ShiftLck  S     C     G    H    N    L    ;+   ]}   Delete     KP #   KP *   KP ,
 * 0x60  Tab       Z     SPC   V    B    M    <,   >.   /?   Copy       KP 0   KP 1   KP 3
 * 0x70  ESC       f1    f2    f3   f5   f6   f8   f9   \    Right      KP 4   KP 5   KP 2
*/
Keyboard::Keyboard(uint8_t dips) //
        : dips_{dips} //
        , auto_scan_enabled_{true} //
        , scan_col_{0} //
        , caps_lock_led_{false} //
        , shift_lock_led_{false} //
        , key_1_{0xff} //
        , key_2_{0xff} //
{
  we_src_ = std::make_shared<data_subscriber_8_bit>(0x08);
  data_src_ = std::make_shared<data_subscriber_8_bit>(0x7f);
  provider_ = std::make_shared<data_provider_8_bit>();
  // CA2 is high until a key is pressed.
  irq_provider_ = std::make_shared<data_provider_8_bit>(0x01);
  cl_led_src_ = std::make_shared<data_subscriber_8_bit>(0x40);
  sl_led_src_ = std::make_shared<data_subscriber_8_bit>(0x80);
}


void Keyboard::handle_auto_scan() {
  // TODO: If auto_scan_enabled_ poll keyb and generate interrupt
  // Needs a CA1 provider
  if (auto_scan_enabled_) {
    bool key_pressed = false;
    for (auto row = 0x10; row != 0x70; row += 0x10) {
      auto test = row | scan_col_;
      if (key_1_ == test || key_2_ == test) {
        spdlog::info("KEYB  : Autoscan detected keypress {:02x} in col {}, raised interrupt",
                     test, scan_col_);
        key_pressed = true;
        break;
      }
    }
    if (key_pressed) {
      // Raise interrupt on CA2
      irq_provider_->provide_data(0x00);
    } else {
      // Or raise IRQ line, not interrupt, nothing to see.
      irq_provider_->provide_data(0x01);
    }
  }
  scan_col_ = (scan_col_+1) % 10;
}

void Keyboard::tick() {
  check_leds();
  check_we();
  if (data_src_->data_changed()) {
    handle_command(data_src_->data());
  }
  handle_auto_scan();
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

bool Keyboard::is_key_pressed(uint8_t key_code) const {
  key_code &= 0x7f;

  // DIP switches
  if (key_code >= KEY_DIP_7 && key_code <= KEY_DIP_0) {
    auto s = 9 - key_code;
    auto tst = 0x01 << s;
    if (dips_ & tst) {
      return true;
    }
    return false;
  }

  // SHIFT and CTL
  if (key_code == KEY_SHIFT) return shift_pressed_;

  if (key_code == KEY_CTL) return ctrl_pressed_;


  // Check for other keys
  if (key_1_ == key_code || key_2_ == key_code) {
    spdlog::info("KEYB  : Checked for keycode {:02x} which was pressed.", key_code);
    return true;
  } else {
    spdlog::info("KEYB  : Checked for keycode {:02x} which was NOT pressed.", key_code);
  }
  return false;
}

void Keyboard::handle_command(uint8_t key_code) const {
  auto scan_result = key_code & 0x7f;
  scan_result |= is_key_pressed(key_code) ? 0x80 : 0x00;
  provider()->provide_data(scan_result);
}

// Store the last two keypresses until they are released
void Keyboard::press_key(uint8_t key) {
  if( key_1_ == key || key_2_ == key) return;
  if (key_1_ == 0xff) {
    key_1_ = key;
    return;
  }
  if (key_2_ == 0xff) {
    key_2_ = key;
    return;
  }
}

void Keyboard::release_key(uint8_t key) {
  if (key_1_ == key) {
    key_1_ = 0xff;
    return;
  }
  if (key_2_ == key) {
    key_2_ = 0xff;
    return;
  }
}

