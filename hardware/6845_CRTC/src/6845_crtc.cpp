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

const uint8_t VDISPENABLE = 1 << 0;
const uint8_t HDISPENABLE = 1 << 1;
const uint8_t SKEWDISPENABLE = 1 << 2;
const uint8_t SCANLINEDISPENABLE = 1 << 3;
const uint8_t USERDISPENABLE = 1 << 4;
const uint8_t FRAMESKIPENABLE = 1 << 5;
const uint8_t EVERYTHINGENABLED =
        VDISPENABLE | HDISPENABLE | SKEWDISPENABLE | SCANLINEDISPENABLE | USERDISPENABLE | FRAMESKIPENABLE;

Crtc::Crtc(uint16_t base_addr) //
        : base_addr_{base_addr} //
        , reg_select_{0}//
        , reg_horz_total_{0} //
        , reg_horz_disp_{0} //
        , reg_horz_sync_pos_{0} //
        , reg_vert_total_{0} //
        , reg_vert_total_adj_{0} //
        , reg_vert_disp_{0} //
        , reg_horz_sync_width_{0} //
        , reg_vert_sync_width_{0} //
        , reg_vert_sync_pos_{0} //
        , reg_ilace_skew_{0} //
        , reg_max_raster_lines_{0} //
        , curs_blink_{0} //
        , curs_blink_rate_{0} //
        , reg_curs_start_raster_{0} //
        , reg_curs_end_raster_{0} //
        , scr_start_addr_{0} //
        , reg_curs_start_addr_{0} //
        , light_pen_pos_{0} //
        , char_cnt_{0} //
        , line_cnt_{0} //
        , raster_cnt_{0} //
        , v_adj_cnt_{0} //
        , hsync_width_cnt_{0} //
        , vsync_width_cnt_{0} //
        , cursor_enabled_{false} //
        , linear_addr_cnt_{0} //
        , h_disp_enable_{0} //
        , v_disp_enable_{0} //
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
  if (addr == CRTC_REG_SELECT) {
    spdlog::error("CRTC: Invalid attempt to read from REG_SELECT");
    return;
  }

  auto data = bus->get_data();
  switch (reg_select_) {
    case REG_CURSOR_POS_HI:
      data = (reg_curs_start_addr_ >> 8) & 0x3f;
      spdlog::get("CRTC")->info("CRTC: Read cursor position hi ({:02x})", data);
      break;
    case REG_CURSOR_POS_LO:
      data = reg_curs_start_addr_ & 0xff;
      spdlog::get("CRTC")->info("CRTC: Read cursor position lo ({:02x})", data);
      break;
    case REG_LPEN_POS_HI:
      data = (light_pen_pos_ >> 8) & 0x3f;
      spdlog::get("CRTC")->info("CRTC: Read light pen position hi ({:02x})", data);
      break;
    case REG_LPEN_POS_LO:
      data = reg_curs_start_addr_ & 0xff;
      spdlog::get("CRTC")->info("CRTC: Read cursor position lo ({:02x})", data);
      break;
    default:
      spdlog::error("CRTC: Attempted illegal read from register {} {}",
                    reg_select_,
                    reg_select_ <= 17 ? register_name_[reg_select_] : "???");
      return;
  }
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
    reg_select_ = reg & 0x1f;
    return;
  }

  auto data = bus->get_data();
  switch (reg_select_) {
    case REG_HORZ_TOTAL:
      reg_horz_total_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to horz_total", data);
      break;

    case REG_HORZ_DISP:
      reg_horz_disp_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to horz_disp", data);
      break;

    case REG_HSYNC_POS:
      reg_horz_sync_pos_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to hsync_pos", data);
      break;

    case REG_SYNCS: {
      auto hw = data & 0xf;
      auto vw = (data >> 4) & 0xf;
      reg_vert_sync_width_ = (vw == 0) ? 16 : vw;
      if (hw != 0) reg_horz_sync_width_ = hw;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to reg_syncs.", data);
      spdlog::get("CRTC")->info("      hsync_pulse is {:02x} {}", reg_horz_sync_width_, (hw == 0) ? "[ignored 0]" : "");
      spdlog::get("CRTC")->info("      vsync_time_ is {:02x}", reg_vert_sync_width_);
    }
      break;

    case REG_VERT_TOTAL:
      reg_vert_total_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to vert_total", data);
      break;

    case REG_VERT_TOTAL_ADJ:
      reg_vert_total_adj_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to vert_total_adj", data);
      break;

    case REG_VERT_TOTAL_DISP:
      reg_vert_disp_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: {:02x} to vert_total_disp", data);
      break;

    case REG_VSYNC_POS:
      reg_vert_sync_pos_ = data;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to vsync_pos", data);
      break;

    case REG_ILD: {
      reg_ilace_skew_ = data;
      r8_interlace_mode_ = reg_ilace_skew_ & 0x03;
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
      r8_display_blanking_delay_ = (reg_ilace_skew_ >> 0x04) & 0x03;
      switch (r8_display_blanking_delay_) {
        case 0:
          spdlog::get("CRTC")->info("      No display blanking delay.");
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
      r8_cursor_blanking_delay_ = (reg_ilace_skew_ >> 0x06) & 0x03;
      switch (r8_cursor_blanking_delay_) {
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
      reg_max_raster_lines_ = data & 0x1f;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to char_scan_lines. Scan lines set to {:02x}", data,
                                reg_max_raster_lines_);
      break;

    case REG_CURSOR_START:
      curs_blink_ = (data >> 6) & 0x01;
      curs_blink_rate_ = (data >> 5) & 0x01;
      reg_curs_start_raster_ = data & 0x1f;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cursor_start_reg.", data);
      spdlog::get("CRTC")->info("      Cursor blink {}", curs_blink_ ? "enabled" : "disabled");
      spdlog::get("CRTC")->info("      blink rate {}", curs_blink_rate_ ? "fast" : "slow");
      spdlog::get("CRTC")->info("      start line {}.", reg_curs_start_raster_);
      break;

    case REG_CURSOR_END:
      reg_curs_end_raster_ = data & 0x1f;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cursor_end_line", reg_curs_end_raster_);
      break;

      // Changes here don't take effect until the next CRTC cycle
    case REG_SCREEN_ADDR_HI:
      scr_start_addr_ = (scr_start_addr_ & 0x00ff) | ((data & 0x3f) << 8);
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to scr_start_addr_hi.", data);
      spdlog::get("CRTC")->info("      Screen start address is {:04x}", scr_start_addr_);
      break;

      // Changes here don't take effect until the next CRTC cycle
    case REG_SCREEN_ADDR_LO:
      scr_start_addr_ = (scr_start_addr_ & 0x3f00) | data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to scr_start_addr_lo.", data);
      spdlog::get("CRTC")->info("      Screen start address is {:04x}", scr_start_addr_);
      break;

    case REG_CURSOR_POS_HI:
      reg_curs_start_addr_ = (reg_curs_start_addr_ & 0xff) | ((data & 0x3f) << 8);
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cur_pos_hi.", data);
      spdlog::get("CRTC")->info("      Cursor pos is x{:04x} : {},{}",
                                reg_curs_start_addr_,
                                ((reg_curs_start_addr_ >> 7) & 0x7f),
                                (reg_curs_start_addr_ & 0x7f));
      break;

    case REG_CURSOR_POS_LO:
      reg_curs_start_addr_ = (reg_curs_start_addr_ & 0x3f00) | data;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cur_pos_lo.", data);
      spdlog::get("CRTC")->info("      Cursor pos is x{:04x} : {},{}",
                                reg_curs_start_addr_,
                                ((reg_curs_start_addr_ >> 7) & 0x7f),
                                (reg_curs_start_addr_ & 0x7f));
      break;

    default:
      spdlog::error("CRTC: Attempted to write {:02x} to illegal register {} {}",
                    data, reg_select_,
                    reg_select_ <= 17 ? register_name_[reg_select_] : "???");
      break;
  }
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
    if (!curs_blink_) cursor_enabled_ = true;
    else {
      /*Bit 5 is the blink timing control bit.
       * When bit 5=0, blink frequency = 16 times the field rate.
       * When bit 5=1, blink frequency = 32 times the field rate.
       */
      cursor_enabled_ =
              (curs_blink_rate_ == 1 && (frame_cnt_ & 0x20)) || (curs_blink_rate_ == 0 && (frame_cnt_ & 0x10));

    }
  }
}


void Crtc::correct_output_addr(uint16_t & addr) {
  auto c0c1 = hw_scroll_addr_->data();
  spdlog::get("CRTC")->info("Output address is {:04x}, correcting by c0c1 {:02x}", addr, c0c1);
  uint16_t addend = 0;
  switch (c0c1) {
    case 0x00:
      addend = 0x4000;
      break;
    case 0x10:
      addend = 0x3000;
      break;
    case 0x20:
      addend = 0x6000;
      break;
    case 0x30:
      addend = 0x5800;
      break;
    default:
      spdlog::get("CRTC")->error("Latch bits for base addr have crazy value ({:02x})", c0c1);
      spdlog::error("Latch bits for base addr have crazy value ({:02x})", c0c1);
      break;
  }
  addr = (addr + addend) & 0x7fff;
  spdlog::get("CRTC")->info("   corrected to {:04x}", addr);
}

void Crtc::latch_address(const std::shared_ptr<Bus> &dram_bus) {
  uint16_t output_addr = 0;
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
    if (linear_addr_cnt_ & 0x1000) correct_output_addr(output_addr);
  }

  spdlog::get("CRTC")->debug("Wrote address {:04x} to DRAM Address bus. Set RW", output_addr);
  dram_bus->set_address(output_addr);
  dram_bus->set_RW();
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
void Crtc::handle_hsync() {
  hsync_width_cnt_ = (hsync_width_cnt_ + 1) & 0x0f;

  /*
   * The original jsbeeb code does a bunch of manipulation of the pixel X counter
   * to split the hblank timing either side of the display area and create a border
   * We should consider doing this in the outer VDU display code instead.
   */
  if (hsync_width_cnt_ == hsync_width_cnt_ >> 1) {
    /*
      // Start at -8 because the +8 is added before the pixel render.
      bitmapX = -8;

      // Half-clock horizontal movement
      if (hsync_width_cnt_ & 1) {
        bitmapX -= 4;
      }

      // The CRT vertical beam speed is constant, so this is actually
      // an approximation that works if hsyncs are spaced evenly.
      bitmapY += 2;

      // If no VSync occurs this frame, go back to the top and force a repaint
      if (bitmapY >= 768) {
        // Arbitrary moment when TV will give up and start flyback in the absence of an explicit VSync signal
        paintAndClear();
      }
      */
  } else if (hsync_width_cnt_ == reg_horz_sync_width_) {
    h_sync_ = false;
  }
}

void Crtc::dispEnableSet(uint8_t flag) {
  dispEnabled_ |= flag;
}

void Crtc::dispEnableClear(uint8_t flag) {
  dispEnabled_ &= ~flag;
}

void Crtc::handle_end_of_frame() {
  line_cnt_ = 0;
  first_raster_ = true;
  next_line_start_addr_ = scr_start_addr_;
  line_start_addr_ = next_line_start_addr_;
  dispEnableSet(VDISPENABLE);

  cursorOnThisFrame_ = !curs_blink_ || (curs_blink_rate_ == 1 && (frame_cnt_ & 0x08)) ||
                       (curs_blink_rate_ == 0 && (frame_cnt_ & 0x10));
  lastRenderWasEven_ = isEvenRender_;
  isEvenRender_ = !(frameCount_ & 1);
  if (!v_sync_) {
    doEvenFrameLogic_ = false;
  }
}

void Crtc::handle_end_of_char_line() {
  line_cnt_ = (line_cnt_ + 1) & 0x7f;
  raster_cnt_ = 0;
  had_vsync_this_raster_ = false;
  dispEnableSet(SCANLINEDISPENABLE);
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
  auto r9Hit = (raster_cnt_ == reg_max_raster_lines_);
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
    raster_cnt_ = (raster_cnt_ + 2) & 0x1e;
  } else {
    raster_cnt_ = (raster_cnt_ + 1) & 0x1fu;
  }
  if (!teletextMode_) {
    // Scanlines 8-15 are off but they display again at 16,
    // mirroring 0-7, and it repeats.
    const bool off = ((raster_cnt_ >> 3) & 1);
    if (off) {
      dispEnableClear(SCANLINEDISPENABLE);
    } else {
      dispEnableSet(SCANLINEDISPENABLE);
    }
  }

  // Reset scanline if necessary.
  if (!in_v_adj_ && r9Hit) {
    handle_end_of_char_line();
  }

  if (endOfMainLatched_ && !end_of_v_adj_latched_) {
    in_v_adj_ = true;
  }

  bool endOfFrame = false;
  if (endOfFrameLatched_) {
    endOfFrame = true;
  }

  if (end_of_v_adj_latched_) {
    in_v_adj_ = false;
    // The "dummy raster" is inserted at the very end of frame,
    // after vertical adjust, for even interlace frames.
    // Testing indicates interlace is checked here, a clock before
    // it is entered or not.
    // Like vertical adjust, C4=R4+1.
    if ((r8_interlace_mode_ & 1) && doEvenFrameLogic_) {
      inDummyRaster_ = true;
      endOfFrameLatched_ = true;
    } else {
      endOfFrame = true;
    }
  }

  if (endOfFrame) {
    endOfMainLatched_ = false;
    end_of_v_adj_latched_ = false;
    endOfFrameLatched_ = false;
    inDummyRaster_ = false;

    handle_end_of_char_line();
    handle_end_of_frame();
  }

  linear_addr_cnt_ = line_start_addr_;

  if (raster_cnt_ == reg_curs_start_raster_) cursorOn_ = true;

  // The teletext SAA5050 chip has its CRS pin connected to RA0, so
  // we need to update it.
  // The external RA0 value is modified in "interlace sync and video"
  // mode to be odd for odd interlace frames.
  auto externalScanline = raster_cnt_;
  if (interlacedSyncAndVideo_ && (frameCount_ & 1)) {
    externalScanline++;
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
  oddClock_ = !oddClock_;
  if (halfClock_ && !oddClock_) {
    return;
  }

  if (h_sync_) handle_hsync();

  // Handle delayed display enable due to skew
  auto displayEnablePos = displayEnableSkew_ + (teletextMode_ ? 2 : 0);
  if (char_cnt_ == displayEnablePos) {
    dispEnableSet(SKEWDISPENABLE);
  }

  // Latch next line screen address in case we are in the last line of a character row
  if (char_cnt_ == reg_horz_disp_) {
    next_line_start_addr_ = linear_addr_cnt_;
  }

  // Handle end of horizontal displayed.
  // Make sure to account for display enable skew.
  // Also, the last scanline character never displays.
  if (char_cnt_ == reg_horz_disp_ + displayEnablePos ||
      char_cnt_ == reg_horz_total_ + displayEnablePos) {
    dispEnableClear(HDISPENABLE | SKEWDISPENABLE);
  }

  // Initiate HSync.
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
  bool isInterlace = (r8_interlace_mode_ & 1);
  // TODO: is this off-by-one? b2 uses regs[0]+1.
  // TODO: does this only hit at the half-scanline or is it a
  // half-scanline counter that starts when an R7 hit is noticed?

  auto halfR0Hit = (char_cnt_ == reg_horz_total_ >> 1);
  auto isVsyncPoint = !isInterlace || !doEvenFrameLogic_ || halfR0Hit;
  bool vSyncEnding = false;
  bool vSyncStarting = false;
  if (v_sync_ && vsync_width_cnt_ == reg_vert_sync_width_ && isVsyncPoint) {
    vSyncEnding = true;
    v_sync_ = false;
  }
  if (line_cnt_ == reg_vert_sync_pos_ && !v_sync_ && !had_vsync_this_raster_ && isVsyncPoint) {
    vSyncStarting = true;
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
  if (vSyncStarting && !vSyncEnding) {
    had_vsync_this_raster_ = true;
    vsync_width_cnt_ = 0;
  }

  if (vSyncStarting || vSyncEnding) {
    // FIXME: Sort teletex out
    //    this.teletext.setDEW(this.inVSync);
  }

  // TODO: JSBeeb Render code removed here, implement in wrapper.
  latch_address(dram_bus);
  //



  // CRTC MA always increments, inside display border or not.
  linear_addr_cnt_ = (linear_addr_cnt_ + 1) & 0x3fff;

  // The Hitachi 6845 decides to end (or never enter) vertical
  // adjust here, one clock after checking whether to enter
  // vertical adjust.
  // In a normal frame, this is C0=2.
  if (check_v_adj_) {
    check_v_adj_ = false;
    if (endOfMainLatched_) {
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
    if (line_cnt_ == reg_vert_total_ && raster_cnt_ == reg_max_raster_lines_) {
      endOfMainLatched_ = true;
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
    dispEnableSet(HDISPENABLE);
  } else {
    char_cnt_ = (char_cnt_ + 1) & 0xff;
  }

  // Handle end of vertical displayed.
  // The Hitachi 6845 will notice this equality at any character,
  // including in the middle of a scanline.
  // An exception is the very first scanline of a frame, where
  // vertical display is always on.
  // We do this after the render and various counter increments
  // because there seems to be a 1 character delay between setting
  // R6=C4 and display actually stopping.
  bool r6Hit = line_cnt_ == reg_vert_disp_;
  if (r6Hit && !first_raster_ && dispEnabled_ & VDISPENABLE) {
    dispEnableClear(VDISPENABLE);
    // Perhaps surprisingly, this happens here. Both cursor
    // blink and interlace cease if R6 > R4.
    frameCount_++;
  }

  // Interlace quirk: an even frame appears to need to see
  // either of an R6 hit or R7 hit in order to activate the
  // dummy raster.
  bool r7Hit = (line_cnt_ == reg_vert_sync_pos_);
  if (r6Hit || r7Hit) {
    doEvenFrameLogic_ = (frameCount_ & 1);
  }
}

/* Force screen paint from top of screen */
// TODO: Updates are not this simple. Once we have basic function working well
// TODO: We need to examine specific behaviour to support screen tears etc.
void Crtc::sync() {
  char_cnt_ = 0;
  raster_cnt_ = 0;
  line_cnt_ = 0;
  v_adj_cnt_ = 0;
  v_disp_enable_ = 1;
  h_disp_enable_ = 1;
}
