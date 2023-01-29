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
#include "clock.h"
#include "data_connectors.h"

#include <cstdint>
#include <string>

class Crtc {
public:
  explicit Crtc(uint16_t base_addr, const std::shared_ptr<Clock> &clk);

  ~Crtc() = default;

  void tick(const std::shared_ptr<Bus> &bus);

  void generate_next_address(const std::shared_ptr<Bus> &dram_bus);

  inline data_subscriber_8_bit_ptr hw_scroll_addr() {
    return hw_scroll_addr_;
  }

  [[nodiscard]] inline uint16_t get_ma() const { return memory_addr_; }

  [[nodiscard]] inline uint16_t get_ra() const { return raster_cnt_; }

  [[nodiscard]] inline bool display_enable() const {
    return (h_disp_enable_ & v_disp_enable_);
  }

  [[nodiscard]] inline bool hsync() const {
    return (hsync_ == 1);
  }

  [[nodiscard]] inline bool vsync() const {
    return (vsync_ == 1);
  }

  [[nodiscard]] inline bool cursor_enabled() const {
    return (cursor_enabled_ == 1);
  }

  /* Force screen paint from top of screen */
  void sync();

private:
  void mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus);

  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);

  uint16_t base_addr_;
  uint8_t reg_select_;

  /* Registers */
  uint8_t horz_total_;
  uint8_t horz_displayed_;
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

  /* Internal counters */
  uint8_t char_cnt_;
  uint8_t line_cnt_;
  uint8_t raster_cnt_;
  uint8_t adj_cnt_;
  uint8_t hsync_width_cnt_;
  uint8_t vsync_width_cnt_;

  /* Outputs */
  bool cursor_enabled_;
  uint16_t memory_addr_;
  uint8_t h_disp_enable_;
  uint8_t v_disp_enable_;
  uint8_t hsync_;
  uint8_t vsync_;

  /* Global clock */
  std::shared_ptr<Clock> clock_;

  std::string register_name_[18];

  /* Data subscribers for other chips */
  data_subscriber_8_bit_ptr hw_scroll_addr_;
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
