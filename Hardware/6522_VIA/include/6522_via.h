#ifndef BEEB_HARDWARE_6522_VIA_INCLUDE_H_
#define BEEB_HARDWARE_6522_VIA_INCLUDE_H_

#include "bus.h"

#include <cstdint>

class Via {
 public:
  explicit Via(uint16_t base_address);

  void tick(Bus &bus);

 private:
  void mmio_read(Bus & bus, uint8_t reg);
  void mmio_write(Bus & bus, uint8_t reg);
  void check_mmio(Bus & bus);

  uint16_t base_address_;

  uint8_t ddra_;
  uint8_t ddrb_;
  uint8_t ier_;

};

#endif //BEEB_HARDWARE_6522_VIA_INCLUDE_H_
