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

#include "keyboard.h"
#include <spdlog/spdlog-inl.h>
#include "spdlog/sinks/basic_file_sink.h"

Keyboard::Keyboard(uint8_t dips) //
        : dips_{dips} //
        , auto_scan_enabled_{true} //
        , scan_col_{0} //
        , caps_lock_led_{false} //
        , shift_lock_led_{false} //
        , key_1_{0xff} //
        , key_2_{0xff} //
        , shift_pressed_{false} //
        , ctrl_pressed_{false} //
{
  we_src_ = std::make_shared<data_subscriber_8_bit>(0x08);
  data_src_ = std::make_shared<data_subscriber_8_bit>(0x7f);
  provider_ = std::make_shared<data_provider_8_bit>();
  irq_provider_ = std::make_shared<data_provider_8_bit>(0x01);
  cl_led_src_ = std::make_shared<data_subscriber_8_bit>(0x40);
  sl_led_src_ = std::make_shared<data_subscriber_8_bit>(0x80);

  try {
    auto logger = spdlog::basic_logger_mt("KEYB", "logs/KEYB.txt", true);
    logger->flush_on(spdlog::level::err);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
  logger_ = spdlog::get("KEYB");
}

/*
 * IFF autoscan, we do not latch any new value from data and we cycle the scan column from 0- 9. PA7 is disabled
 * IFF not autoscan we DO latch PA0-PA3 into row and POA4-PA6 into column and enable PA7 output AS WELL AS NAND
 * BUT ROW 0 is not scanned.
 */
void Keyboard::tick() {
  check_leds();
  check_we();

  if (!auto_scan_enabled_) {
    if (data_src_->data_changed()) {
      auto data = data_src_->data();
      logger_->debug("Rx data {:02x}", data);
      scan_col_ = (data & 0x0f);
      check_pa7(data);
    }
  }
  check_irq();

  if (auto_scan_enabled_) {
    // The LS163 counts up to 16 in binary but the 7445 only passes on columns 0-9
    scan_col_ = (scan_col_ + 1) % 15;
  }
}

/**
 * If a key is being pressed, we raise an IRQ for it by pulling the line low.
 */
void Keyboard::check_irq() {
  if ((key_1_ != 0xff) && ((key_1_ & 0x0f) == scan_col_)) {
    irq_provider_->provide_data(0x00);
    return;
  }
  if ((key_2_ != 0xff) && ((key_2_ & 0x0f) == scan_col_)) {
    irq_provider_->provide_data(0x00);
  }
}

/**
 * If auto scan is not enabled, set the scan column and set PA7 if
 * a key that is pressed matches the provided key code
 */
void Keyboard::check_pa7(uint8_t key_code) const {
  assert(!auto_scan_enabled_);
  // Do nothing for scan columns from 9 - 15
  key_code &= 0x7f;

  if (scan_col_ > 9) return;

  // DIP switches
  if (key_code >= KEY_DIP_7 && key_code <= KEY_DIP_0) {
    auto s = 9 - key_code;
    auto tst = 0x01 << s;
    provider()->provide_data((dips_ & tst) ? 0x80 | key_code : 0);
    return;
  }

  // SHIFT and CTL
  if (key_code == KEY_SHIFT) {
    provider()->provide_data(shift_pressed_ ? (0x80 | key_code) : 0);
    return;
  }

  if (key_code == KEY_CTL) {
    provider()->provide_data(ctrl_pressed_ ? (0x80 | key_code) : 0);
    return;
  }

  /* Check against any pressed keys
   * We already masked off the top bit so it won't match empty keys
   */
  if (key_1_ == key_code) {
    provider()->provide_data(0x80 | key_code);
    return;
  }
  if (key_2_ == key_code) {
    provider()->provide_data(0x80 | key_code);
  }

  provider()->provide_data(0x00);
}

/**
 * Check for write enable which toggles autoscan
 */
void Keyboard::check_we() {
  if (!we_src_->data_changed()) return;
  auto enable = we_src_->data();
  if (enable == auto_scan_enabled_) return;

  auto_scan_enabled_ = enable;
  logger_->debug("Autoscan {}", auto_scan_enabled_ ? "enabled" : "disabled");
}

void log_state(const std::string &led_name, bool old_state, bool new_state) {
  std::string action = (old_state == new_state) ? "still" : "turned";
  std::string state = new_state ? "on" : "off";
  spdlog::debug("{} LED {} {}", led_name, action, state);
}

/*
 *
 */
void Keyboard::check_leds() {
  if (cl_led_src_->data_changed()) {
    auto on = (cl_led_src_->data() == 0x00);
    log_state("Check leds, CAPS Lock", caps_lock_led_, on);
    caps_lock_led_ = on;
  }
  if (sl_led_src_->data_changed()) {
    auto on =  (sl_led_src_->data() == 0x00);
    log_state("Check leds, SHIFT Lock", shift_lock_led_, on);
    shift_lock_led_ = on;
  }
}


// Store the last two keypresses until they are released
void Keyboard::press_key(uint8_t key) {
  if( key == KEY_SHIFT) {shift_pressed_ = true;return;}
  if( key == KEY_CTL) {ctrl_pressed_ = true;return;}
  if (key_1_ == key || key_2_ == key) return;
  if (key_1_ == 0xff) {
    logger_->debug("Key {:02x} pressed");
    key_1_ = key;
    return;
  }
  if (key_2_ == 0xff) {
    logger_->debug("Key {:02x} pressed");
    key_2_ = key;
    return;
  }
}

void Keyboard::release_key(uint8_t key) {
  if( key == KEY_SHIFT) {shift_pressed_ = false;return;}
  if( key == KEY_CTL) {ctrl_pressed_ = false;return;}

  if (key_1_ == key) {
    key_1_ = key_2_;
    key_2_ = 0xff;
    logger_->debug("Key {:02x} released");
    return;
  }
  if (key_2_ == key) {
    key_2_ = 0xff;
    logger_->debug("Key {:02x} released");
    return;
  }
}

