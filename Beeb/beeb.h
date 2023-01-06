//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_SRC_BEEB_H_
#define M6502_SRC_BEEB_H_

#include "../M6502/include/memory.h"
#include "../M6502/include/m6502.h"
#include "../6522VIA/system_via.h"
#include "../Keyboard/keyboard.h"

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
