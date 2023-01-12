//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef BEEB_HARDWARE_ACIA_6850_H_
#define BEEB_HARDWARE_ACIA_6850_H_

#include "bus.h"

#include <cstdint>

class Acia {
 public:
  Acia();

  void tick(Bus &bus);

 private:
};

#endif // BEEB_HARDWARE_ACIA_6850_H_
