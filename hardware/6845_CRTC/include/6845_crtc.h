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
  explicit Crtc(uint16_t base_addr);

  ~Crtc() = default;

  void tick(const std::shared_ptr<Bus> &bus);

  void generate_next_address(const std::shared_ptr<Bus> &dram_bus);

  inline data_subscriber_8_bit_ptr hw_scroll_addr() {
    return hw_scroll_addr_;
  }

  [[nodiscard]] inline uint16_t get_ma() const { return linear_addr_cnt_; }

  [[nodiscard]] inline uint16_t get_ra() const { return raster_cnt_; }

  [[nodiscard]] inline data_provider_8_bit_ptr irq_provider() const { return irq_provider_; }


  [[nodiscard]] inline bool display_enable() const {
    return (h_disp_enable_ & v_disp_enable_) /* && (( raster_cnt_ & 0x08) ==0)*/;
  }

  [[nodiscard]] inline bool hsync() const {
    return (hsync_ == 1);
  }

  [[nodiscard]] inline uint16_t last_generated_address() const { return last_generated_address_; }

  [[nodiscard]] inline bool vsync() const {
    return (vsync_ == 1);
  }

  [[nodiscard]] inline bool cursor_enabled() const {
    return (cursor_enabled_ == 1);
  }

  /* Force screen paint from top of screen */
  void sync();

private:
  void latch_address(const std::shared_ptr<Bus> &dram_bus);

  void handle_cursor();

  void mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus);

  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);

  uint16_t base_addr_;
  uint8_t reg_select_;

  /* Registers */
  uint8_t horz_total_;        //  R0
  uint8_t horz_disp_;         //  R1
  uint8_t horz_sync_pos_;     //  R2
  uint8_t horz_sync_width_;   //  R3
  uint8_t vert_total_;        //  R4
  uint8_t vert_total_adj_;    //  R5
  uint8_t vert_disp_;         //  R6
  uint8_t vert_sync_pos_;     //  R7
  uint8_t vert_sync_width_;   //  R3
  uint8_t ilace_skew_;        //  R8
  uint8_t max_raster_lines_;  //  R9
  uint8_t curs_start_raster_; //  R10
  uint8_t curs_end_raster_;   //  R11
  uint16_t scr_start_addr_;   //  R12/R13
  uint16_t curs_start_addr_;  //  R14/R15
  uint8_t curs_blink_;
  uint8_t curs_blink_rate_;
  uint16_t light_pen_pos_;    //  R16

  /* Internal counters */
  uint8_t char_cnt_;
  uint8_t line_cnt_;
  uint8_t raster_cnt_;
  uint8_t adj_cnt_;
  uint8_t hsync_width_cnt_;
  uint8_t vsync_width_cnt_;
  uint8_t frame_cnt_;
  uint16_t raster_start_addr_;

  bool raster_ended_;
  bool line_ended_;
  bool screen_ended_;

  /* Outputs */
  bool cursor_enabled_;
  uint16_t linear_addr_cnt_;
  uint8_t h_disp_enable_;
  uint8_t v_disp_enable_;
  uint8_t hsync_;
  uint8_t vsync_;

  uint16_t last_generated_address_;

  /* Global clock */
  std::shared_ptr<Clock> clock_;

  std::string register_name_[18];

  /* Data subscribers for other chips */
  data_subscriber_8_bit_ptr hw_scroll_addr_;

  data_provider_8_bit_ptr irq_provider_;
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
