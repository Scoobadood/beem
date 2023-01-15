/*
 * 5C094 (Video ULA)
 * The video processor device (IC6) is a custom uncommitted logic array (ULA) developed especially
 * for use in the BBC Micro.
 * At the end of each CRTC 250nS access period, it latches the byte from the RAM and, according to
 * the display mode in operation, serialises the byte into 1 bit stream of 8 bits or 2 bit streams
 * of 4 bits etc.
 * In this way, display modes varying in width from 640 pixels in 2 colours to 160 pixels in 8 colours, which may
 * or may not be flashing, can be produced.
 * Also, in the video processor is a high speed piece of static RAM called a palette. This memory can be programmed
 * to define the relationship between the logical colour produced by the RAM and the physical colour which will
 * appear on the display. Thus, in a 640 pixel mode, the two colours to appear on the display need not be black
 * and white, they may be, say, red and blue. Note that the data in RAM is unchanged by the palette, it is
 * the mapping onto physical colours which changes.
 * Modes 0 through 6 in the Micro are so-called bitmapped screens, which allow for raster graphics. With these screens,
 * each pixel on the screen corresponds directly with one, two or four bits in the video memory. This method of
 * producing video screens is expensive in memory, involving a minimum of 8 kilobytes for the display.
 * 
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

  void render_to(uint8_t * buffer, uint32_t buffer_length);

 private:
  void mmio_write(uint16_t addr, Bus &bus);
  void write_palette(uint8_t data);
  void process_data_to_image(uint8_t data);

  uint16_t base_addr_;

  uint8_t palette_[16];
  std::string colour_name_[16];

  uint8_t vula_ctl_;

  // Cycles 0,1,2,3. We only have the bus on 2,3
  uint8_t tick_count_;

  // Render target
  uint8_t * buffer_;
  uint32_t buffer_length_;
  uint32_t buffer_idx_;
  uint8_t clk_freq_;
  uint8_t cursor_width_;
};

#endif // BEEB_HARDWARE_5C095_VULA_H
