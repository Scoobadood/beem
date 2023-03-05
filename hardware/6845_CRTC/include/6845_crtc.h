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

const uint8_t VDISPENABLE = 1 << 0;
const uint8_t HDISPENABLE = 1 << 1;
const uint8_t SKEWDISPENABLE = 1 << 2;
const uint8_t SCANLINEDISPENABLE = 1 << 3;
const uint8_t USERDISPENABLE = 1 << 4;
const uint8_t FRAMESKIPENABLE = 1 << 5;
const uint8_t EVERYTHINGENABLED =
        VDISPENABLE | HDISPENABLE | SKEWDISPENABLE | SCANLINEDISPENABLE | USERDISPENABLE | FRAMESKIPENABLE;

class Crtc {
public:
  explicit Crtc(uint16_t base_addr);

  ~Crtc() = default;

  void tick(const std::shared_ptr<Bus> &bus);

  void generate_next_address(const std::shared_ptr<Bus> &dram_bus);

  [[nodiscard]] inline data_subscriber_8_bit_ptr hw_scroll_addr() { return hw_scroll_addr_; }

  [[nodiscard]] inline data_provider_8_bit_ptr irq_provider() const { return irq_provider_; }


  [[nodiscard]] inline bool display_enable() const {
    return (dispEnabled_ & (HDISPENABLE | VDISPENABLE)) == (HDISPENABLE | VDISPENABLE);
  }

  [[nodiscard]] inline bool hsync() const { return h_sync_; }

  [[nodiscard]] inline uint16_t last_generated_address() const { return last_generated_address_; }

  [[nodiscard]] inline bool vsync() const { return v_sync_; }

  [[nodiscard]] inline bool cursor_enabled() const { return (cursor_enabled_ == 1) && display_enable(); }

  /* Force screen paint from top of screen */
  void sync();

private:
  void latch_address(const std::shared_ptr<Bus> &dram_bus);

  void dispEnableSet(uint8_t flag);

  void dispEnableClear(uint8_t flag);

  void handle_cursor();

  void handle_hsync();

  void handle_end_of_frame();

  void handle_end_of_char_line();

  void handle_end_of_scan_line();

  void correct_output_addr(uint16_t &addr);

  void mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus);

  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);

  uint16_t base_addr_;
  uint8_t reg_select_;

  /* Registers */
  uint8_t reg_horz_total_;            //  R0
  uint8_t reg_horz_disp_;             //  R1
  uint8_t reg_horz_sync_pos_;         //  R2
  uint8_t r3_horz_sync_pulse_width_;  //  R3
  uint8_t r3_vert_sync_pulse_width_;  //  R3
  uint8_t reg_vert_total_;            //  R4
  uint8_t reg_vert_total_adj_;        //  R5
  uint8_t reg_vert_disp_;             //  R6
  uint8_t reg_vert_sync_pos_;         //  R7
  uint8_t reg_ilace_skew_;            //  R8
  bool r8_ilace_sync_and_video_;      //  R8
  bool r8_is_interlace_;              //  R8
  uint8_t r8_interlace_mode_;
  uint8_t r8_display_enable_skew_;       //  R8
  uint8_t r8_cursor_delay_;           //  R8

  uint8_t reg_max_raster_lines_;  //  R9
  uint8_t reg_curs_start_raster_; //  R10
  uint8_t reg_curs_end_raster_;   //  R11
  uint16_t scr_start_addr_;   //  R12/R13
  uint16_t reg_curs_start_addr_;  //  R14/R15
  uint8_t r10_curs_blink_;
  uint8_t r10_curs_blink_rate_;
  uint16_t light_pen_pos_;    //  R16

  /* Internal counters */
  uint8_t frame_cnt_;

  /* JSBeeb */

  int16_t char_cnt_;
  uint8_t hsync_width_cnt_;
  bool h_sync_;

  int16_t raster_cnt_;
  bool first_raster_;

  int16_t line_cnt_;
  int16_t line_start_addr_;
  int16_t next_line_start_addr_;
  int32_t vsync_width_cnt_;
  bool v_sync_;
  bool had_vsync_this_raster_;

  int v_adj_cnt_;
  bool in_v_adj_;
  bool check_v_adj_;
  bool end_of_v_adj_latched_;

  int16_t frameCount_;
  bool doEvenFrameLogic_;
  bool cursorOnThisFrame_;
  bool endOfFrameLatched_;

  bool cursorOn_;
  bool cursorOff_;
  int cursorDrawIndex_;

  uint8_t dispEnabled_;
  int32_t r8_display_enable_skew_;
  bool isEvenRender_;
  bool lastRenderWasEven_;
  bool teletextMode_;
  bool endOfMainLatched_;
  bool inDummyRaster_;


  /* Outputs */
  bool cursor_enabled_;
  uint16_t linear_addr_cnt_;
  uint8_t h_disp_enable_;
  uint8_t v_disp_enable_;

  uint16_t last_generated_address_;

  /* Global clock */
  std::shared_ptr<Clock> clock_;

  std::string register_name_[18];

  /* Data subscribers for other chips */
  data_subscriber_8_bit_ptr hw_scroll_addr_;

  data_provider_8_bit_ptr irq_provider_;
};

#endif // BEEB_HARDWARE_6845_CRTC_H_
