//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_SRC_BEEB_H_
#define M6502_SRC_BEEB_H_

#include "memory.h"
#include "m6502.h"
#include "system_via.h"
#include "keyboard.h"

class Beeb {
 public:
  Beeb();
  void tick();

 private:
  uint64_t clock_;
  Memory * memory_;
  Keyboard * keyboard_;
  SystemVia * system_via_;
  M6502 * cpu_;
};

#endif //M6502_SRC_BEEB_H_
