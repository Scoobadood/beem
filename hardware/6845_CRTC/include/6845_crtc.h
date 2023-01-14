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

  std::string register_name_[18];
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
