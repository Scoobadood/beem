//
// Created by Dave Durbin on 30/12/2022.
//

#ifndef BEEB_HARDWARE_5C095_VULA_H
#define BEEB_HARDWARE_5C095_VULA_H

#include "bus.h"

#include <vector>

class VideoUla {
 public:
  VideoUla(uint16_t base_addr);
  ~VideoUla();

  void tick(Bus &bus);

 private:
  void mmio_read(Bus &bus);
  void mmio_write(Bus &bus);

  uint16_t base_addr_;
  uint8_t mode_;
  uint8_t *pixel_data_rgb_;
};

#endif // BEEB_HARDWARE_5C095_VULA_H
