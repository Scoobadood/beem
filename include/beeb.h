//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_SRC_BEEB_H_
#define M6502_SRC_BEEB_H_

#include "memory.h"
#include "cpu.h"
#include "via.h"
#include "keyboard.h"

class Beeb {
 public:
  Beeb();
  void tick();

 private:
  uint64_t clock_;
  Memory * memory_;
  Keyboard * keyboard_;
  Cpu * cpu_;
};

#endif //M6502_SRC_BEEB_H_
