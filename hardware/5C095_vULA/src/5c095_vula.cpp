#include "5c095_vula.h"

#include <spdlog/spdlog-inl.h>

const uint8_t VCR_WO = 0x00;

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

VideoUla::VideoUla(uint16_t base_addr) //
    : base_addr_{base_addr} //
{}

void VideoUla::mmio_read(Bus & bus) {
  spdlog::warn( "vULA: Unimplemented read from {:04x}",bus.get_address());
}

void VideoUla::mmio_write(Bus & bus) {
  spdlog::warn( "vULA: Unimplemented write of {:02x} to {:04x}", bus.get_data(), bus.get_address());
}

void VideoUla::tick(Bus &bus) {
  auto addr = bus.get_address();
  if (addr < base_addr_ || addr > base_addr_) return;
  if( bus.tst_RW()) {
    mmio_read(bus);
  } else {
    mmio_write(bus);
  }
}