//
// Created by Dave Durbin on 14/1/2023.
//

#ifndef BEEB_HARDWARE_5C095_VULA_MODE_H_
#define BEEB_HARDWARE_5C095_VULA_MODE_H_

#include <cstdint>
struct Mode {
  uint8_t cells_x;
  uint8_t cells_y;
  uint16_t pixels_x;
  uint16_t pixels_y;
  uint8_t colours;
  uint16_t video_ram_start;
  uint16_t video_ram_end;
  bool is_graphics;
  Mode(uint8_t cells_x,
       uint8_t cells_y,
       uint16_t pixels_x,
       uint16_t pixels_y,
       uint8_t colours,
       uint16_t video_ram_start,
       uint16_t video_ram_end,
       bool is_graphics
  ) //
      : cells_x{cells_x} //
      , cells_y{cells_y} //
      , pixels_x{pixels_x} //
      , pixels_y{pixels_y} //
      , colours{colours} //
      , video_ram_start{video_ram_start} //
      , video_ram_end{video_ram_end} //
      , is_graphics{is_graphics} //
  {

  }
};

Mode modes[8] = {
    {80, 32, 640, 256, 2, 0x3000, 0x7fff, true},
    {40, 32, 320, 256, 4, 0x3000, 0x7fff, true},
    {20, 32, 160, 256, 8, 0x3000, 0x7fff, true},
    {80, 25, 640, 200, 2, 0x4000, 0x7fff, false},
    {40, 32, 320, 256, 2, 0x5800, 0x7fff, true},
    {20, 32, 160, 256, 4, 0x5800, 0x7fff, true},
    {40, 25, 320, 200, 2, 0x6000, 0x7fff, false},
    {40, 25, 480, 500, 8, 0x7c00, 0x7fff, false}
};

#endif // BEEB_HARDWARE_5C095_VULA_MODE_H_
