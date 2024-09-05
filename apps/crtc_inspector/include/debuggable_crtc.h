#ifndef LIBS_BEEB_APPS_CRTC_INSPECTOR_INCLUDE_DEBUGGABLE_CRTC_H_
#define LIBS_BEEB_APPS_CRTC_INSPECTOR_INCLUDE_DEBUGGABLE_CRTC_H_

#include <6845_crtc.h>

class DebuggableCrtc : public Crtc {
 public:
  DebuggableCrtc();
  void set_register(uint8_t reg, uint8_t value);

  uint8_t get_reg_horz_total() { return reg_horz_total_; }
  uint8_t get_reg_horz_disp() { return reg_horz_disp_; }
  uint8_t get_horz_sync_pos() { return reg_horz_sync_pos_; }
  uint8_t get_sync_width() { return r3_vert_sync_pulse_width_ << 4 | r3_horz_sync_width_; }
  uint8_t get_reg_vert_total() { return reg_vert_total_; }
  uint8_t get_reg_vert_adj() { return reg_vert_total_adj_; }
  uint8_t get_reg_vert_disp() { return reg_vert_disp_; }
  uint8_t get_vert_sync_pos() { return reg_vert_sync_pos_; }
  uint8_t get_reg_interlace() { return reg_ilace_and_delay_; }
  uint8_t get_reg_char_rasters() { return reg_rasters_per_char_; }
  uint8_t get_reg_cursor() { return reg_curs_start_raster_; }
  uint8_t get_reg_cursor_end() { return reg_curs_end_raster_; }
  uint8_t get_reg_screen_start_hi() { return reg_scr_start_addr_ >> 8; }
  uint8_t get_reg_screen_start_lo() { return reg_scr_start_addr_ & 0xff; }
  uint8_t get_reg_cursor_start_hi() { return reg_curs_start_addr_ >> 8; }
  uint8_t get_reg_cursor_start_lo() { return reg_curs_start_addr_ & 0xff; }
  uint8_t get_reg_light_pen_hi() { return reg_light_pen_pos_ >> 8; }
  uint8_t get_reg_light_pen_lo() { return reg_light_pen_pos_ & 0xff; }

  uint16_t get_output_addr() { return last_generated_address_; }
  uint16_t get_char_cnt() { return char_cnt_; }
  uint16_t get_linear_cnt() { return linear_addr_cnt_; }
  uint16_t get_raster_in_char() { return raster_cnt_; }
  uint16_t get_character_row() { return character_line_cnt_; }

  bool is_end_of_scanline() { return (char_cnt_ == reg_horz_total_); }
  bool is_end_of_row() {
    return (char_cnt_ == reg_horz_total_) && (raster_cnt_ == reg_rasters_per_char_);
  }
 private:
  std::shared_ptr<Bus> bus_;
};

#endif //LIBS_BEEB_APPS_CRTC_INSPECTOR_INCLUDE_DEBUGGABLE_CRTC_H_
