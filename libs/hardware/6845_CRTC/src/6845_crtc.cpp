/*
 */
#include "6845_crtc.h"
#include "data_connectors.h"
#include <spdlog/spdlog-inl.h>
#include "spdlog/sinks/basic_file_sink.h"

/* MMIO */
const uint8_t CRTC_REG_SELECT = 0x00;
const uint8_t CRTC_READ_WRITE = 0x01;

/* Internal registers */
const uint8_t REG_HORZ_TOTAL = 0x00;
const uint8_t REG_HORZ_DISP = 0x01;
const uint8_t REG_HSYNC_POS = 0x02;
const uint8_t REG_SYNCS = 0x03;
const uint8_t REG_VERT_TOTAL = 0x04;
const uint8_t REG_VERT_TOTAL_ADJ = 0x05;
const uint8_t REG_VERT_TOTAL_DISP = 0x06;
const uint8_t REG_VSYNC_POS = 0x07;
const uint8_t REG_ILD = 0x08;
const uint8_t REG_CHAR_SCAN_LINES = 0x09;
const uint8_t REG_CURSOR_START = 0x0A;
const uint8_t REG_CURSOR_END = 0x0b;
const uint8_t REG_SCREEN_ADDR_HI = 0x0c;
const uint8_t REG_SCREEN_ADDR_LO = 0x0d;
const uint8_t REG_CURSOR_POS_HI = 0x0e;
const uint8_t REG_CURSOR_POS_LO = 0x0f;
const uint8_t REG_LPEN_POS_HI = 0x10;
const uint8_t REG_LPEN_POS_LO = 0x11;

Crtc::Crtc(uint16_t base_addr) //
    :
    reg_horz_total_{0} //
    , reg_horz_disp_{0} //
    , reg_horz_sync_pos_{0} //
    , r3_horz_sync_width_{0} //
    , r3_vert_sync_pulse_width_{0} //
    , reg_vert_total_{0} //
    , reg_vert_total_adj_{0} //
    , reg_vert_disp_{0} //
    , reg_vert_sync_pos_{0} //
    , reg_ilace_and_delay_{0} //
    , r8_interlace_mode_{0}//
    , r8_is_interlace_{false}//
    , r8_display_enable_skew_{0}//
    , r8_cursor_delay_{0}//
    , reg_rasters_per_char_{0} //
    , reg_curs_start_raster_{0} //
    , r10_curs_blink_{0} //
    , r10_curs_blink_rate_{0} //
    , reg_curs_end_raster_{0} //
    , reg_scr_start_addr_{0} //
    , reg_curs_start_addr_{0} //
    , reg_light_pen_pos_{0} //
    , frame_count_{0} //
    , char_cnt_{0} //
    , hsync_width_cnt_{0} //
    , raster_cnt_{0} //
    , character_line_cnt_{0} //
    , vsync_width_cnt_{0} //
    , v_adj_cnt_{0} //
    , linear_addr_cnt_{0} //
    , start_of_frame_{false} //
    , h_sync_{false} //
    , first_raster_{false} //
    , v_sync_{false} //
    , had_vsync_this_raster_{false} //
    , in_v_adj_{false} //
    , check_v_adj_{false} //
    , end_of_v_adj_latched_{false} //
    , cursorOn_{false} //
    , cursorOff_{false} //
    , cursor_enabled_{false} //
    , line_start_addr_{0} //
    , next_line_start_addr_{0} //
    , last_generated_address_{0} //
    , display_enabled_{FRAME_SKIP_ENABLE} //
    , base_addr_{base_addr}//
    , selected_register_{0}//
    , do_even_frame_logic_{false}//
    , end_of_frame_latched_{false}//
    , in_dummy_raster_{false}//
    , is_even_frame_{false}//
    , last_frame_was_even_{false} //
    , cursor_on_this_frame_{false} //
    , teletext_mode_{false}//
    , end_of_main_latched_{false}//
    , register_name_
        {
            "Horizontal total", "Horizontal displayed characters",
            "Horizontal sync position", "Horizontal sync width/Vertical sync time",
            "Vertical total", "Vertical total adjust", "Vertical displayed characters",
            "Vertical sync position",
            "Interlace/Display delay/Cursor delay",
            "Scan lines per character", "Cursor start line and blink type", "Cursor end line",
            "Screen start address hi", "Screen start address lo",
            "Cursor position hi", "Cursor position lo",
            "Light pen position hi", "Light pen position lo"
        }//
{
  hw_scroll_addr_ = std::make_shared<data_subscriber_8_bit>(0x30);
  try {
    auto logger = spdlog::basic_logger_mt("CRTC", "logs/crtc-log.txt", true);
    logger->flush_on(spdlog::level::debug);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }

  irq_provider_ = std::make_shared<data_provider_8_bit>(0x00);
}

void Crtc::mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  if (addr != CRTC_READ_WRITE) {
    spdlog::error("CRTC: Invalid attempt to read from {:04x}", addr);
    bus->set_data(0);
    return;
  }

  uint8_t data;
  spdlog::get("CRTC")->info("CRTC: Trying to read from ({}) {}", selected_register_,
                            selected_register_ < register_name_->size()
                            ? register_name_[selected_register_]
                            : "???");
  switch (selected_register_) {
    case REG_SCREEN_ADDR_HI:
      data = (reg_scr_start_addr_ >> 8) & 0x3f;
      break;
    case REG_SCREEN_ADDR_LO:
      data = reg_scr_start_addr_ & 0xff;
      break;
    case REG_CURSOR_POS_HI:
      data = (reg_curs_start_addr_ >> 8) & 0x3f;
      break;
    case REG_CURSOR_POS_LO:
      data = reg_curs_start_addr_ & 0xff;
      break;
    case REG_LPEN_POS_HI:
      data = (reg_light_pen_pos_ >> 8) & 0x3f;
      break;
    case REG_LPEN_POS_LO:
      data = reg_light_pen_pos_ & 0xff;
      break;
    default:
      spdlog::error("      Read not supported");
      return;
  }
  spdlog::get("CRTC")->info("      read {:02x}", data);
  bus->set_data(data);
}

void Crtc::mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  if (addr == CRTC_REG_SELECT) {
    auto reg = bus->get_data();
    if (reg > 17) {
      spdlog::error("CRTC: Selected invalid register {}", reg);
    } else {
      spdlog::get("CRTC")->info("CRTC: Selected {} register", register_name_[reg]);
    }
    selected_register_ = reg & 0x1f;
    return;
  }

  auto data = bus->get_data();
  spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to {}", data, register_name_[selected_register_]);
  switch (selected_register_) {
    case REG_HORZ_TOTAL:
      reg_horz_total_ = data;
      break;

    case REG_HORZ_DISP:
      reg_horz_disp_ = data;
      break;

    case REG_HSYNC_POS:
      reg_horz_sync_pos_ = data;
      break;

    case REG_SYNCS: {
      r3_vert_sync_pulse_width_ = (data >> 4) & 0xf; // NB 0 == 16
      auto hw = (data & 0xf);
      if (hw != 0) {
        r3_horz_sync_width_ = hw;
      }
      spdlog::get("CRTC")->info("      hsync_pulse_width is {:02x} {}", r3_horz_sync_width_,
                                (hw == 0) ? "[ignored 0]" : "");
      spdlog::get("CRTC")->info("      vsync_pulse_width is {:02x}", r3_vert_sync_pulse_width_);
    }
      break;

    case REG_VERT_TOTAL:
      reg_vert_total_ = data & 0x7f;
      spdlog::get("CRTC")->info("      reg_vert_total_ : {:02x}", reg_vert_total_);
      break;

    case REG_VERT_TOTAL_ADJ:
      reg_vert_total_adj_ = data & 0x1f;
      spdlog::get("CRTC")->info("      reg_vert_total_adj_ : {:02x}", reg_vert_total_adj_);
      break;

    case REG_VERT_TOTAL_DISP:
      reg_vert_disp_ = data & 0x7f;
      spdlog::get("CRTC")->info("      reg_vert_disp_ : {:02x}", reg_vert_disp_);
      break;

    case REG_VSYNC_POS:
      reg_vert_sync_pos_ = data & 0x7f;
      spdlog::get("CRTC")->info("      reg_vert_sync_pos_ : {:02x}", reg_vert_sync_pos_);
      break;

    case REG_ILD: {
      // Mask out unused bits
      reg_ilace_and_delay_ = data & 0xf3;
      r8_interlace_mode_ = reg_ilace_and_delay_ & 0x03;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to ILD.", data);
      switch (r8_interlace_mode_) {
        case 0:
        case 1:
          spdlog::get("CRTC")->info("      Normal (non-interlaced) sync mode.");
          break;
        case 2:
          spdlog::get("CRTC")->info("      Interlace sync.");
          break;
        case 3:
          spdlog::get("CRTC")->info("      Interlace sync and video.");
          break;
      }
      auto skew = (reg_ilace_and_delay_ & 0x30) >> 4;
      if (skew < 3) {
        r8_display_enable_skew_ = skew;
        set_disp_enable(USER_DISP_ENABLE);
      } else {
        clear_disp_enable(USER_DISP_ENABLE);
      }
      switch (r8_display_enable_skew_) {
        case 0:
          spdlog::get("CRTC")->info("      No display enable delay.");
          break;
        case 1:
          spdlog::get("CRTC")->info("      One character display blanking delay.");
          break;
        case 2:
          spdlog::get("CRTC")->info("      Two character display blanking delay.");
          break;
        case 3:
          spdlog::get("CRTC")->info("      Video output disabled.");
          break;
      }
      r8_is_interlace_ = (reg_ilace_and_delay_ & 0x01);

      r8_cursor_delay_ = (reg_ilace_and_delay_ >> 0x06) & 0x03;
      switch (r8_cursor_delay_) {
        case 0:
          spdlog::get("CRTC")->info("      No cursor blanking delay.");
          break;
        case 1:
          spdlog::get("CRTC")->info("      One character cursor blanking delay.");
          break;
        case 2:
          spdlog::get("CRTC")->info("      Two character cursor blanking delay.");
          break;
        case 3:
          spdlog::get("CRTC")->info("      Cursor output disabled.");
          break;
      }
    }
      break;

    case REG_CHAR_SCAN_LINES:
      reg_rasters_per_char_ = data & 0x1f;
      spdlog::get("CRTC")->info("      reg_max_raster_lines_ : {:02x}", reg_rasters_per_char_);
      break;

    case REG_CURSOR_START:
      r10_curs_blink_ = (data >> 6) & 0x01;
      r10_curs_blink_rate_ = (data >> 5) & 0x01;
      reg_curs_start_raster_ = data & 0x1f;
      spdlog::get("CRTC")->info("      Cursor blink {}", r10_curs_blink_ ? "enabled" : "disabled");
      spdlog::get("CRTC")->info("      blink rate {}", r10_curs_blink_rate_ ? "fast" : "slow");
      spdlog::get("CRTC")->info("      start line {}.", reg_curs_start_raster_);
      break;

    case REG_CURSOR_END:
      reg_curs_end_raster_ = data & 0x1f;
      spdlog::get("CRTC")->info("      reg_curs_end_raster_ : {:02x}", reg_curs_end_raster_);
      break;

      // Changes here don't take effect until the next CRTC cycle
      // As the CRTC address is latched at the start of frame.
    case REG_SCREEN_ADDR_HI:
      reg_scr_start_addr_ = (reg_scr_start_addr_ & 0x00ff) | ((data & 0x3f) << 8);
      spdlog::get("CRTC")->info("      Screen start address is {:04x}", reg_scr_start_addr_);
      break;

    case REG_SCREEN_ADDR_LO:
      reg_scr_start_addr_ = (reg_scr_start_addr_ & 0x3f00) | data;
      spdlog::get("CRTC")->info("      Screen start address is {:04x}", reg_scr_start_addr_);
      break;

    case REG_CURSOR_POS_HI:
      reg_curs_start_addr_ = (reg_curs_start_addr_ & 0xff) | ((data & 0x3f) << 8);
      spdlog::get("CRTC")->info("      Cursor pos is {:04x}", reg_curs_start_addr_);
      break;

    case REG_CURSOR_POS_LO:
      reg_curs_start_addr_ = (reg_curs_start_addr_ & 0x3f00) | data;
      spdlog::get("CRTC")->info("      Cursor pos is {:04x}", reg_curs_start_addr_);
      break;

    default:
      spdlog::error("CRTC: Attempted to write {:02x} to illegal register {}", data, selected_register_);
      break;
  }
}

void Crtc::reset() {
  last_generated_address_ = 0;
  selected_register_ = 0;
  char_cnt_ = 0;
  character_line_cnt_ = 0;
  raster_cnt_ = 0;
  v_adj_cnt_ = 0;
  hsync_width_cnt_ = 0;
  vsync_width_cnt_ = 0;
  cursor_enabled_ = false;
  linear_addr_cnt_ = 0;
  first_raster_ = true;
  start_of_frame_ = true;
  is_even_frame_ = true;
  display_enabled_ = FRAME_SKIP_ENABLE;
}

/**
 * Poll the address bus for data and read it if my address is present.
 * This is tied to the BBC 1MHzE clock falling edge.
 * @param bus
 */
void Crtc::tick(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < base_addr_ || addr > (base_addr_ + CRTC_READ_WRITE)) {
    return;
  }
  spdlog::get("CRTC")->info("CRTC: CS Addr: {:04x}, RW:{}, {}.",
                            addr,
                            (bus->tst_RW() ? "R" : "W"),
                            (bus->tst_RW() ? "" : fmt::format("D:{:02x}", bus->get_data())));
  addr -= base_addr_;
  if (bus->tst_RW()) {
    mmio_read(addr, bus);
  } else {
    mmio_write(addr, bus);
  }
}

void Crtc::handle_cursor() {
  /*
   * Cursor handling extrapolated from here:
   * https://www.cpcwiki.eu/index.php/VHDL_implementation_of_the_6845
   */
  cursor_enabled_ = false;
  if (linear_addr_cnt_ == reg_curs_start_addr_) {
    if (raster_cnt_ >= reg_curs_start_raster_ && raster_cnt_ <= reg_curs_end_raster_) {
    }
    if (!r10_curs_blink_) cursor_enabled_ = true;
    else {
      /*Bit 5 is the blink timing control bit.
       * When bit 5=0, blink frequency = 16 times the field rate.
       * When bit 5=1, blink frequency = 32 times the field rate.
       */
      cursor_enabled_ =
          (r10_curs_blink_rate_ == 1 && (frame_count_ & 0x20)) || (r10_curs_blink_rate_ == 0 && (frame_count_ & 0x10));

    }
  }
}

/*
 Inputs	Outputs	Meanings
 MA12  C1 C0	Amount 	      Restart	MODEs
                to subtract   address
  0	   x  x	    0             n/a       0..6
  1	   0  0	    &4000         &4000	    3
  1    0  1	    &2000         &6000	    6
  1    1  0	    &5000         &3000	    0,1,2
  1    1  1	    &2800         &5800	    4,5

 */
void Crtc::wrap_output_addr(uint16_t &addr) {
  auto c0c1 = hw_scroll_addr_->data() >> 4;
  spdlog::get("CRTC")->info("Output address is {:04x}, correcting by c0c1 {:02x}", addr, c0c1);
  uint16_t subtrahend = 0;
  switch (c0c1) {
    case 0x00:
      subtrahend = 0x4000;
      break;
    case 0x01:
      subtrahend = 0x2000;
      break;
    case 0x02:
      subtrahend = 0x5000;
      break;
    case 0x03:
      subtrahend = 0x2800;
      break;
    default:
      spdlog::get("CRTC")->error("Latch bits for base addr have crazy value ({:02x})", c0c1);
      spdlog::error("Latch bits for base addr have crazy value ({:02x})", c0c1);
      break;
  }
  addr = (addr - subtrahend);
  spdlog::get("CRTC")->info("   corrected to {:04x}", addr);
}

void Crtc::latch_address(const std::shared_ptr<Bus> &dram_bus) {
  uint16_t output_addr;
  if (linear_addr_cnt_ & 0x2000) {
    // Mode 7 chunky addressing mode if MA13 set.
    // Address offset by scanline is ignored.
    // On model B only, there's a quirk for reading 0x3c00.
    // See: http://www.retrosoftware.co.uk/forum/viewtopic.php?f=73&t=1011
    output_addr = linear_addr_cnt_ & 0x3ff;
    if (linear_addr_cnt_ & 0x800) {
      output_addr |= 0x7c00;
    } else {
      output_addr |= 0x3c00;
    }
  } else {
    output_addr = (raster_cnt_ & 0x07) | (linear_addr_cnt_ << 3);

    // Perform screen address wrap around if MA12 set
    if (linear_addr_cnt_ & 0x1000) {
      wrap_output_addr(output_addr);
    }
  }

  spdlog::get("CRTC")->debug("Wrote address {:04x} to DRAM Address bus. Set RW", output_addr);
  dram_bus->set_address(output_addr);
  dram_bus->set_RW();


//   DEBUG: Logging the generated address here
//  static int32_t cnt = 0;
//  static int32_t row = 0;
//  if (output_addr == reg_scr_start_addr_ * 8) {
//    std::cout << std::endl
//              << "----------------------------------------------------------------------------------------------------"
//              << std::endl;
//    row = 0;
//    cnt = 0;
//  }
//  std::cout << "0x" << std::hex << std::setw(4) << output_addr << " ";
//  if (++cnt == reg_horz_total_ + 1) {
//    cnt = 0;
//    std::cout << std::endl;
//    if (++row == reg_rasters_per_char_ + 1) {
//      row = 0;
//      std::cout << std::endl;
//    }
//  }
//  if (output_addr == 0x7ff8) {
//    int p = 0;
//  }
  // End of DEBUG

  last_generated_address_ = output_addr;

  spdlog::get("BusDance")->debug("CRTC: Writing vram address, expects to own bus. DRAM bus {:04x} {:02x} {} {}",
                                 dram_bus->get_address(),
                                 dram_bus->get_data(),
                                 dram_bus->tst_RW() ? "R" : "W",
                                 dram_bus->tst_SYNC() ? "SYN" : "   ",
                                 dram_bus->tst_RST() ? "RST" : "");
}

/**
 * Reset hsync when the width count has passed.
 */
void Crtc::maybe_handle_hsync() {
  if (!h_sync_) return;

  hsync_width_cnt_ = (hsync_width_cnt_ + 1) & 0x0f;
  if (hsync_width_cnt_ == r3_horz_sync_width_) {
    h_sync_ = false;
  }
}

void Crtc::set_disp_enable(uint8_t flag) {
  display_enabled_ |= flag;
}

void Crtc::clear_disp_enable(uint8_t flag) {
  display_enabled_ &= ~flag;
}

void Crtc::handle_end_of_frame() {
  character_line_cnt_ = 0;
  first_raster_ = true;
  next_line_start_addr_ =
      reg_scr_start_addr_;// this.nextLineStartAddr = (this.regs[13] | (this.regs[12] << 8)) & 0x3fff;
  line_start_addr_ = next_line_start_addr_;
  set_disp_enable(VSYNC_DISP_ENABLE);

  cursor_on_this_frame_ = !r10_curs_blink_ || (r10_curs_blink_rate_ == 1 && (frame_count_ & 0x08)) ||
      (r10_curs_blink_rate_ == 0 && (frame_count_ & 0x10));
  last_frame_was_even_ = is_even_frame_;
  is_even_frame_ = !(frame_count_ & 1);
  if (!v_sync_) {
    do_even_frame_logic_ = false;
  }
}

void Crtc::handle_end_of_char_line() {
  character_line_cnt_ = static_cast<int16_t>((character_line_cnt_ + 1) & 0x7f);
  raster_cnt_ = 0;
  had_vsync_this_raster_ = false;
  set_disp_enable(SCANLINE_DISP_ENABLE);
  cursorOn_ = false;
  cursorOff_ = false;
}

void Crtc::handle_end_of_scan_line() {
  // End of scanline is the most complicated and quirky area of the
  // 6845. A lot of different states and outcomes are possible.
  // From the start of the frame, we traverse various states
  // linearly, with most optional:
  // - Normal rendering.
  // - Last scanline of normal rendering (vertical adjust pending).
  // - Vertical adjust.
  // - Last scanline of vertical adjust (dummy raster pending).
  // - Dummy raster. (This is for interlace timing.)
  first_raster_ = false;

  if (raster_cnt_ == reg_curs_end_raster_) cursorOff_ = true;

  vsync_width_cnt_ = (vsync_width_cnt_ + 1) & 0x0f;

  // Pre-counter increment compares and logic.
  auto r9Hit = (raster_cnt_ == reg_rasters_per_char_);
  if (r9Hit) {
    // An R9 hit always loads a new character row address, even if
    // we're in vertical adjust!
    // Note that an R9 hit inside vertical adjust does not further
    // increment the vertical counter, but entry into vertical
    // adjust does.
    line_start_addr_ = next_line_start_addr_;
  }

  // Increment scanline.
  bool interlacedSyncAndVideo_ = (r8_interlace_mode_ == 3);
  if (interlacedSyncAndVideo_) {
    raster_cnt_ = static_cast<int16_t>((raster_cnt_ + 2) & 0x1e);
  } else {
    raster_cnt_ = static_cast<int16_t>((raster_cnt_ + 1) & 0x1fu);
  }
  if (!teletext_mode_) {
    // Scanlines 8-15 are off but they display again at 16,
    // mirroring 0-7, and it repeats.
    const bool off = ((raster_cnt_ >> 3) & 1);
    if (off) {
      clear_disp_enable(SCANLINE_DISP_ENABLE);
    } else {
      set_disp_enable(SCANLINE_DISP_ENABLE);
    }
  }

  // Reset scanline if necessary.
  if (!in_v_adj_ && r9Hit) {
    handle_end_of_char_line();
  }

  if (end_of_main_latched_ && !end_of_v_adj_latched_) {
    in_v_adj_ = true;
  }

  bool endOfFrame = false;
  if (end_of_frame_latched_) {
    endOfFrame = true;
  }

  if (end_of_v_adj_latched_) {
    in_v_adj_ = false;
    // The "dummy raster" is inserted at the very end of frame,
    // after vertical adjust, for even interlace frames.
    // Testing indicates interlace is checked here, a clock before
    // it is entered or not.
    // Like vertical adjust, C4=R4+1.
    if ((r8_interlace_mode_ & 1) && do_even_frame_logic_) {
      in_dummy_raster_ = true;
      end_of_frame_latched_ = true;
    } else {
      endOfFrame = true;
    }
  }

  if (endOfFrame) {
    end_of_main_latched_ = false;
    end_of_v_adj_latched_ = false;
    end_of_frame_latched_ = false;
    in_dummy_raster_ = false;
    start_of_frame_ = true;

    handle_end_of_char_line();
    handle_end_of_frame();
  }

  linear_addr_cnt_ = line_start_addr_;

  if (raster_cnt_ == reg_curs_start_raster_) cursorOn_ = true;

  // The teletext SAA5050 chip has its CRS pin connected to RA0, so
  // we need to update it.
  // The external RA0 value is modified in "interlace sync and video"
  // mode to be odd for odd interlace frames.
  auto external_scanline = raster_cnt_;
  if (interlacedSyncAndVideo_ && (frame_count_ & 1)) {
    external_scanline++;
  }
  // FIXME: Sort teletext out
  //  teletext.setRA0(!!(externalScanline & 1));
}

/**
 * Generate the next Video RAM address. This code is ported from the JSBeeb implementation
 * In particular the actual rendering of bitmaps is removed but the logic around registers
 * and counters is preserved.
 *
 * @param dram_bus
 */
void Crtc::generate_next_address(const std::shared_ptr<Bus> &dram_bus) {
  if (start_of_frame_) {
    linear_addr_cnt_ = reg_scr_start_addr_;
    line_start_addr_ = linear_addr_cnt_;
    start_of_frame_ = false;
  }

  // Turn off if on and pulse width elapsed
  maybe_handle_hsync();

  // Handle delayed display enable due to skew
  // Note that DE_POS is 0 in all modes other than TTX where it's 3
  auto display_enable_pos = r8_display_enable_skew_ + (teletext_mode_ ? 2 : 0);
  if (char_cnt_ == display_enable_pos) {
    set_disp_enable(SKEW_DISP_ENABLE);
  }

  // If we're in the last column of a character row
  // latch next line screen address
  // TODO: Could probably do this only once
  if (char_cnt_ == reg_horz_disp_) {
    next_line_start_addr_ = linear_addr_cnt_;
  }

  // Handle end of horizontal displayed.
  // Make sure to account for display enable skew.
  // Also, the last scanline character never displays.
  if (char_cnt_ == reg_horz_disp_ + display_enable_pos ||
      char_cnt_ == reg_horz_total_ + display_enable_pos) {
    clear_disp_enable(HSYNC_DISP_ENABLE | SKEW_DISP_ENABLE);
  }

  // Check for and if necessary initiate HSync.
  if (char_cnt_ == reg_horz_sync_pos_ && !h_sync_) {
    h_sync_ = true;
    hsync_width_cnt_ = 0;
  }

  // Handle VSync.
  // Half-line interlace timing is shown nicely in figure 13 here:
  // http://bitsavers.trailing-edge.com/components/motorola/_dataSheets/6845.pdf
  // Essentially, on even frames, vsync raise / lower triggers at
  // the mid-scanline, and then a dummy scanline is also added
  // at the end of vertical adjust.
  // Without interlace, frames are 312 scanlines. With interlace,
  // both odd and even frames are 312.5 scanlines.
  // TODO: is this off-by-one? b2 uses regs[0]+1.
  // TODO: does this only hit at the half-scanline or is it a half-scanline counter that starts when an R7 hit is noticed?
  auto hit_half_horz_total = (char_cnt_ == (reg_horz_total_ >> 1));
  auto should_trigger_v_sync = !r8_is_interlace_ || !do_even_frame_logic_ || hit_half_horz_total;
  bool v_sync_ending = false;
  bool v_sync_starting = false;
  if (v_sync_ && (vsync_width_cnt_ == r3_vert_sync_pulse_width_) && should_trigger_v_sync) {
    v_sync_ending = true;
    v_sync_ = false;
    irq_provider_->provide_data(0x00);
  }
  if ((character_line_cnt_ == reg_vert_sync_pos_) && !v_sync_ && !had_vsync_this_raster_ && should_trigger_v_sync) {
    v_sync_starting = true;
    v_sync_ = true;
  }

  // A vsync will initiate at any character and scanline position,
  // provided there isn't one in progress and provided there
  // wasn't already one in this character row.
  // This is an interesting finding, on a real model B.
  // One further emulated quirk is that in the corner case of a
  // vsync ending and starting at the same time, the vsync
  // pulse continues uninterrupted. The vsync pulse counter will
  // continue counting up and wrap at 16.
  if (v_sync_starting && !v_sync_ending) {
    had_vsync_this_raster_ = true;
    vsync_width_cnt_ = 0;
    // Raise vsync
    irq_provider_->provide_data(0xff);
  }

  if (v_sync_starting || v_sync_ending) {
    // FIXME: Sort teletex out
    //    this.teletext.setDEW(this.inVSync);
  }

  // Only latch address inside border
  auto insideBorder =
      (display_enabled_ & (HSYNC_DISP_ENABLE | VSYNC_DISP_ENABLE)) == (HSYNC_DISP_ENABLE | VSYNC_DISP_ENABLE);
  if ((insideBorder /* || this.cursorDrawIndex) */ && (display_enabled_ & FRAME_SKIP_ENABLE))) {

    if (last_generated_address_ == reg_curs_start_addr_ &&
        cursorOn_ && !cursorOff_ && char_cnt_ < reg_horz_disp_) {
      // TODO: Set the cursor drawing pos
      // this.cursorDrawIndex = 3 - ((this.regs[8] >>> 6) &3);
    }


    // Read data from address pointer if both horizontal and vertical display enabled.
    // TODO: JSBeeb Render code removed here, implement in wrapper.
  }



  // CRTC MA always increments, inside display border or not.
  // Mask because R12/13 are 14 bits long only
  linear_addr_cnt_ = (linear_addr_cnt_ + 1) & 0x3fff;

  // The Hitachi 6845 decides to end (or never enter) vertical
  // adjust here, one clock after checking whether to enter
  // vertical adjust.
  // In a normal frame, this is C0=2.
  if (check_v_adj_) {
    check_v_adj_ = false;
    if (end_of_main_latched_) {
      if (v_adj_cnt_ == reg_vert_total_adj_) {
        end_of_v_adj_latched_ = true;
      }
      v_adj_cnt_ = (v_adj_cnt_ + 1) & 0x1f;
    }
  }

  // The Hitachi 6845 appears to latch some form of "last scanline
  // of the frame" state. As shown by Twisted Brain, changing R9
  // from 0 to 6 on the last scanline of the frame does not
  // prevent a new frame from starting.
  // Testing indicates that the latch is set here at exactly C0=1.
  // See also: http://www.cpcwiki.eu/forum/programming/crtc-detailed-operation/msg177585/
  if (char_cnt_ == 1) {
    if (character_line_cnt_ == reg_vert_total_ && raster_cnt_ == reg_rasters_per_char_) {
      end_of_main_latched_ = true;
      v_adj_cnt_ = 0;
    }
    // The very next cycle (be it on this same scanline or the
    // next) is used for checking the vertical adjust counter.
    check_v_adj_ = true;
  }

  // Handle horizontal total.
  if (char_cnt_ == reg_horz_total_) {
    handle_end_of_scan_line();
    char_cnt_ = 0;
    set_disp_enable(HSYNC_DISP_ENABLE);
  } else {
    char_cnt_ = static_cast<int16_t>((char_cnt_ + 1) & 0xff);
  }

  // Handle end of vertical displayed.
  // The Hitachi 6845 will notice this equality at any character,
  // including in the middle of a scanline.
  // An exception is the very first scanline of a frame, where
  // vertical display is always on.
  // We do this after the render and various counter increments
  // because there seems to be a 1 character delay between setting
  // R6=C4 and display actually stopping.
  bool r6Hit = character_line_cnt_ == reg_vert_disp_;
  if (r6Hit && !first_raster_ && display_enabled_ & VSYNC_DISP_ENABLE) {
    clear_disp_enable(VSYNC_DISP_ENABLE);
    // Perhaps surprisingly, this happens here. Both cursor-blink
    // and interlace cease if R6 > R4.
    frame_count_++;
  }

  // Interlace quirk: an even frame appears to need to see
  // either of an R6 hit or R7 hit in order to activate the
  // dummy raster.
  bool r7Hit = (character_line_cnt_ == reg_vert_sync_pos_);
  if (r6Hit || r7Hit) {
    do_even_frame_logic_ = (frame_count_ & 1);
  }

  latch_address(dram_bus);

}