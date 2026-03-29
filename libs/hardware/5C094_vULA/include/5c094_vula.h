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
#include "6845_crtc.h"
#include "clock.h"

#include <utility>
#include <vector>
#include <string>

#include <spdlog/spdlog.h>

class VideoUla {
 public:
  explicit VideoUla(uint16_t base_addr);

  ~VideoUla() = default;

  void set_clock(std::shared_ptr<Clock> clk) { clock_ = std::move(clk); }

  void set_crtc(const std::shared_ptr<Crtc> &crtc);

  void tick(const std::shared_ptr<Bus> &main_bus, const std::shared_ptr<Bus> &dram_bus);

  /* Read the current RGB value as 00rrggbb */
  [[nodiscard]] uint32_t rgb() const;

  void dump(std::shared_ptr<spdlog::logger> &logger);

 private:
  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);

  void write_palette(uint8_t data);

  void latch_new_data(const std::shared_ptr<Bus> &dram_bus);

  void maybe_drive_crtc(const std::shared_ptr<Bus> &dram_bus);

  bool time_to_shift();

  void reset_shift_clk();

  void process_data();

  uint16_t base_addr_;

  uint8_t palette_[16];
  std::string colour_name_[16];

  uint8_t vula_ctl_;

  uint8_t red_;
  uint8_t grn_;
  uint8_t blu_;

  uint8_t shift_clk_;
  uint8_t shift_countdown_;
  uint8_t crtc_clk_;
  uint8_t cursor_width_;
  uint8_t curr_data_;
  uint16_t last_latched_addr_;
  uint16_t last_latched_data_;
  uint8_t num_shifts_;

  std::shared_ptr<Clock> clock_;
  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<spdlog::logger> bus_dance_logger_;
};

#endif // BEEB_HARDWARE_5C095_VULA_H
