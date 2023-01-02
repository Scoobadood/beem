//
// Created by Dave Durbin on 2/1/2023.
//

#include "keyboard.h"
#include "via.h"

void key_pressed(uint8_t key_code, bool shift, bool ctl) {
  if ((shift || ctl)
      && (key_code != KEY_SPACE)
      && (key_code != KEY_0)
      && (key_code != KEY_AT)
      && (key_code != KEY_DELETE)
      && (key_code != KEY_POUND)) {
    key_code -= (shift ? 1 : 0);
    key_code -= (ctl ? 1 : 0);
  }
}
