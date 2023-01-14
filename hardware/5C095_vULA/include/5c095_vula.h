/*
 * https://beebwiki.mdfs.net/Video_ULA
 */
#ifndef BEEB_HARDWARE_5C095_VULA_H
#define BEEB_HARDWARE_5C095_VULA_H

#include "bus.h"

#include <vector>
#include <string>

class VideoUla {
 public:
  explicit VideoUla(uint16_t base_addr);
  ~VideoUla() = default;

  void tick(Bus &bus);

 private:
  void mmio_write(uint16_t addr, Bus &bus);
  void write_palette(uint8_t data);

  uint16_t base_addr_;

  uint8_t palette_[16];
  std::string colour_name_[16];

  uint8_t vula_ctl_;
};

#endif // BEEB_HARDWARE_5C095_VULA_H
