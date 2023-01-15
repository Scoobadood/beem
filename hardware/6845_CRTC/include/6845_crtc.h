/*
 * 6845 (CRT controller, CRTC)
 * This is the heart of the BBC Micro's video circuitry. Its major function is that of displaying the video data
 * in memory on a raster scan display device such as a television or monitor. As a bonus, the sequential nature
 * of accessing the system RAM for the video display refreshes all the DRAM storage.
 * The CRTC does not interfere with CPU access to the memory, as they operate on alternate phases of the system clock.
 * The 6845 is responsible for producing the correct format on the display device, positioning the cursor, and
 * monitoring the light pen input. Other video functions involving colour and Teletext are dealt with by the
 * video ULA (IC6) and the Teletext Character Generator (IC5).
 *
 * http://www.6502.org/users/andre/hwinfo/crtc/crtc.html
 * https://en.wikipedia.org/wiki/Motorola_6845
 */
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

  // Lines MA0- MA13
  inline uint16_t get_ma()const {return char_addr_;}

  // Lines RA0-RA4
  inline uint8_t get_ra()const {return row_addr_;}

 private:
  void mmio_read(uint16_t addr, Bus &bus);
  void mmio_write(uint16_t addr, Bus &bus);

  uint16_t base_addr_;
  uint8_t reg_select_;

  uint8_t horz_total_; // Nht
  uint8_t horz_displayed_; //Nhd
  uint8_t hsync_pos_;
  uint8_t vert_total_;
  uint8_t vert_total_adj_;
  uint8_t vert_total_disp_;
  uint8_t hsync_pulse_width_;
  uint8_t vsync_pulse_time_;
  uint8_t vsync_pos_;
  uint8_t ild_;
  uint8_t char_scan_lines_;
  uint8_t cursor_blink_;
  uint8_t cursor_blink_rate_;
  uint8_t cursor_start_line_;
  uint8_t cursor_end_line_;
  uint16_t screen_start_;
  uint16_t cursor_pos_;
  uint16_t light_pen_pos_;

  /* Address generation */
  uint16_t scanline_char_addr_;
  uint16_t char_addr_;
  uint16_t row_addr_;
  uint8_t display_enable_;

  std::string register_name_[18];
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
