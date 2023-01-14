//
// Created by Dave Durbin on 14/1/2023.
//

#ifndef BEEB_HARDWARE_6845_CRTC_H_
#define BEEB_HARDWARE_6845_CRTC_H_

#include "bus.h"

#include <cstdint>
#include <string>

class Crtc {
 public:
  explicit Crtc(uint16_t base_addr);
  ~Crtc() = default;

  void tick(Bus &bus);

 private:
  void mmio_read(uint16_t addr, Bus &bus);
  void mmio_write(uint16_t addr, Bus &bus);
  
  uint16_t base_addr_;
  uint8_t reg_select_;
  std::string register_name_[18];
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
