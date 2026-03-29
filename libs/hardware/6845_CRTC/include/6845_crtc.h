/*
 * 6845 (CRT controller, CRTC)
 * This is the heart of the BBC Micro's video circuitry. Its major function is that of displaying the video data
 * in memory on a raster scan display device such as a television or monitor. As a bonus, the sequential nature
 * of accessing the system RAM for the video display refreshes all the DRAM storage.
 *
 * The CRTC does not interfere with CPU access to the memory, as they operate on alternate phases of the system clock.
 * The 6845 is responsible for producing the correct format on the display device, positioning the cursor, and
 * monitoring the light pen input.
 *
 * Other video functions involving colour and Teletext are dealt with by the
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
#include <spdlog/spdlog.h>

const uint8_t VSYNC_DISP_ENABLE = 1 << 0;
const uint8_t HSYNC_DISP_ENABLE = 1 << 1;
const uint8_t SKEW_DISP_ENABLE = 1 << 2;
const uint8_t SCANLINE_DISP_ENABLE = 1 << 3;
const uint8_t USER_DISP_ENABLE = 1 << 4;
const uint8_t FRAME_SKIP_ENABLE = 1 << 5;
const uint8_t EVERYTHING_ENABLED =
    VSYNC_DISP_ENABLE | HSYNC_DISP_ENABLE | SKEW_DISP_ENABLE | SCANLINE_DISP_ENABLE | USER_DISP_ENABLE
        | FRAME_SKIP_ENABLE;

class Crtc {
 public:
  explicit Crtc(uint16_t base_addr);

  ~Crtc() = default;

  void tick(const std::shared_ptr<Bus> &bus);

  void generate_next_address(const std::shared_ptr<Bus> &dram_bus);

  [[nodiscard]] inline data_subscriber_8_bit_ptr hw_scroll_addr() { return hw_scroll_addr_; }

  [[nodiscard]] inline data_provider_8_bit_ptr irq_provider() const { return irq_provider_; }

  [[nodiscard]] inline bool display_enable() const {
    return ((display_enabled_ & EVERYTHING_ENABLED)
        == EVERYTHING_ENABLED);// & (HSYNC_DISP_ENABLE | VSYNC_DISP_ENABLE)) == (HSYNC_DISP_ENABLE | VSYNC_DISP_ENABLE);
  }

  void reset();
  [[nodiscard]] inline bool hsync() const { return h_sync_; }

  [[nodiscard]] inline uint16_t last_generated_address() const { return last_generated_address_; }

  [[nodiscard]] inline bool vsync() const { return v_sync_; }

  [[nodiscard]] inline bool cursor_enabled() const { return (cursor_enabled_ == 1) && display_enable(); }

 protected:
  // Total characters -1
  //    Modes 0-3: 127
  //    Modes 4-7: 63
  uint8_t reg_horz_total_;            //  R0
  // Characters displayed (not -1)
  //    Modes 0-3: 80
  //    Modes 4-7: 40
  uint8_t reg_horz_disp_;             //  R1
  // Position of the horizontal sync pulse in character bytes.
  // Increasing the HSP pushes the entire display to the left,
  // Decrementing it pushes the entire display to the right.
  //    Modes 0-3: 98
  //    Modes 4-6: 49
  //    Mode 7   : 51
  uint8_t reg_horz_sync_pos_;         //  R2
  // These set the horizontal and vertical sync pulse widths, as a character byte count
  //   bits 3-0 set the horizontal sync pulse width, with zero meaning 16
  //   bits 7-0 set the vertical sync pulse width, with zero meaning 16
  //   Modes 0-3 : hsync width is 8, vsync height is 2
  //   Modes 4-7 : hsync width is 4, vsync height is 2
  uint8_t r3_horz_sync_width_;  //  R3
  uint8_t r3_vert_sync_pulse_width_;  //  R3
  // Number of character lines that represents the full vertical screen size
  // Covering the displayed and non-displayed area.
  // The total number of lines usually includes a fraction part, which is set with R5
  //   Modes 0-2, 4,5 : 38
  //   Modes  3,6,7   : 30
  uint8_t reg_vert_total_;            //  R4
  // This 5-bit register sets the fractional adjustment to the total screen height
  // Measured in pixel lines.
  // Increasing VTA moves the screen upwards, decreasing it moves the screen downwards.
  //    Modes 0-2,4,5 : 0
  //    Modes 3,6,7   : 2
  uint8_t reg_vert_total_adj_;        //  R5
  // This 7-bit register is the number of visible displayed character lines.
  //    Mode 0-2,4,5 : 32
  //    Mode 3,6,7   : 25
  uint8_t reg_vert_disp_;             //  R6
  // This 7-bit registers sets the position of the vertical sync pulse in character lines.
  // Increasing the VSP pushes the entire display upwards, decrementing it pushes the entire display downwards.
  //    Modes 0-2,4,5 : 34
  //    Modes 3,6,7   : 27
  uint8_t reg_vert_sync_pos_;         //  R7

  uint8_t reg_ilace_and_delay_;       //  R8
  uint8_t r8_interlace_mode_;         //  R8
  bool r8_is_interlace_;              //  R8
  uint8_t r8_display_enable_skew_;    //  R8
  uint8_t r8_cursor_delay_;           //  R8

  // Number of scan lines per character
  //   Modes 0-2,4,5 : 7
  //   Modes 3,6     : 9
  //   Mode  7       : 18
  uint8_t reg_rasters_per_char_;      //  R9
  uint8_t reg_curs_start_raster_;     //  R10
  uint8_t r10_curs_blink_;
  uint8_t r10_curs_blink_rate_;
  uint8_t reg_curs_end_raster_;       //  R11
  uint16_t reg_scr_start_addr_;       //  R12/R13
  uint16_t reg_curs_start_addr_;      //  R14/R15
  uint16_t reg_light_pen_pos_;        //  R16

  /* Internal counters */
  uint8_t frame_count_;
  int16_t char_cnt_;
  uint8_t hsync_width_cnt_;
  int16_t raster_cnt_;
  int16_t character_line_cnt_;
  int32_t vsync_width_cnt_;
  int32_t v_adj_cnt_;
  uint16_t linear_addr_cnt_;

  bool start_of_frame_;
  bool h_sync_;
  bool first_raster_;
  bool v_sync_;
  bool had_vsync_this_raster_;
  bool in_v_adj_;
  bool check_v_adj_;
  bool end_of_v_adj_latched_;
  bool cursorOn_;
  bool cursorOff_;
  bool cursor_enabled_;

  uint16_t line_start_addr_;
  uint16_t next_line_start_addr_;
  uint16_t last_generated_address_;
  uint8_t display_enabled_;

 private:
  void latch_address(const std::shared_ptr<Bus> &dram_bus);

  void set_disp_enable(uint8_t flag);

  void clear_disp_enable(uint8_t flag);

  void handle_cursor();

  void maybe_handle_hsync();

  void handle_end_of_frame();

  void handle_end_of_char_line();

  void handle_end_of_scan_line();

  void wrap_output_addr(uint16_t &addr);

  void mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus);

  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);

  uint16_t base_addr_;
  uint8_t selected_register_;

  bool do_even_frame_logic_;
  bool end_of_frame_latched_;
  bool in_dummy_raster_;

  bool is_even_frame_;
  bool last_frame_was_even_;
  bool cursor_on_this_frame_;
  bool teletext_mode_;
  bool end_of_main_latched_;

  /* Global clock */
  std::shared_ptr<Clock> clock_;

  std::string register_name_[18];

  /* Data subscribers for other chips */
  data_subscriber_8_bit_ptr hw_scroll_addr_;

  data_provider_8_bit_ptr irq_provider_;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<spdlog::logger> bus_dance_logger_;
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
