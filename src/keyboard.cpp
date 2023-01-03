//
// Created by Dave Durbin on 2/1/2023.
//

#include "keyboard.h"
#include "via.h"

Keyboard::Keyboard() {
  dips_ = 0x00;
}

bool Keyboard::key_pressed(uint8_t key_code, bool shift, bool ctl) const {
  // Handle DIP polling
  if (key_code >= KEY_DIP_7 && key_code <= KEY_DIP_0) {
    auto s = 9 - key_code;
    auto tst = 0x01 << s;
    return ((dips_ & tst) == 1);
  }

  if ((shift || ctl)
      && (key_code != KEY_SPACE)
      && (key_code != KEY_0)
      && (key_code != KEY_AT)
      && (key_code != KEY_DELETE)
      && (key_code != KEY_POUND)) {
    key_code -= (shift ? 1 : 0);
    key_code -= (ctl ? 1 : 0);
  }
  //TODO Fix
  return false;
}
