/*
 * https://en.wikipedia.org/wiki/Motorola_6845
 */
#include "6845_crtc.h"
#include <spdlog/spdlog-inl.h>

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
    : base_addr_{base_addr} //
    , reg_select_{0}//
    , register_name_
          {
              "Horizontal total", "Horizontal displayed characters",
              "Horizontal sync position", "Horizontal sync width/Vertical sync time",
              "Vertical total", "Vertical total adjust", "Vertical displayed characters", "Vertical sync position",
              "Interlace/Display delay/Cursor delay",
              "Scan lines per character", "Cursor start line and blink type", "Cursor end line",
              "Screen start address hi", "Screen start address lo",
              "Cursor position hi", "Cursor position lo",
              "Light pen position hi", "Light pen position lo"
          }//
{}

void Crtc::mmio_read(uint16_t addr, Bus &bus) {
  if (addr == CRTC_REG_SELECT) {
    spdlog::error("CRTC: Invalid attempt to read from REG_SELECT");
    return;
  }

  auto data = bus.get_data();
  switch (reg_select_) {
    case REG_CURSOR_POS_HI:data = (cursor_pos_ >> 8) & 0x3f;
      spdlog::info("CRTC: Read cursor position hi ({:02x})", data);
      break;
    case REG_CURSOR_POS_LO:data = cursor_pos_ & 0xff;
      spdlog::info("CRTC: Read cursor position lo ({:02x})", data);
      break;
    case REG_LPEN_POS_HI:data = (light_pen_pos_ >> 8) & 0x3f;
      spdlog::info("CRTC: Read light pen position hi ({:02x})", data);
      break;
    case REG_LPEN_POS_LO:data = cursor_pos_ & 0xff;
      spdlog::info("CRTC: Read cursor position lo ({:02x})", data);
      break;
    default:
      spdlog::error("CRTC: Attempted illegal read from register {} {}",
                    reg_select_,
                    reg_select_ <= 17 ? register_name_[reg_select_] : "???");
      return;
  }
  bus.set_data(data);
}

void Crtc::mmio_write(uint16_t addr, Bus &bus) {
  if (addr == CRTC_REG_SELECT) {
    auto reg = bus.get_data();
    if (reg > 17) {
      spdlog::error("CRTC: Selected invalid register {}", reg);
    } else {
      spdlog::info("CRTC: Selected {} register", register_name_[reg]);
    }
    reg_select_ = reg & 0x1f;
    return;
  }

  auto data = bus.get_data();
  switch (reg_select_) {
    case REG_HORZ_TOTAL:horz_total_ = data;
      spdlog::info("CRTC: Set horz_total to {:02x}", data);
      break;
    case REG_HORZ_DISP:horz_displayed_ = data;
      spdlog::info("CRTC: Set horz_disp to {:02x}", data);
      break;
    case REG_HSYNC_POS:hsync_pos_ = data;
      spdlog::info("CRTC: Set hsync_pos to {:02x}", data);
      break;
    case REG_SYNCS:hsync_pulse_width_ = data & 0xf;
      vsync_pulse_time_ = (data >> 4) & 0xf;
      spdlog::info("CRTC: Set hsync_pulse to {:02x}, vsync_time_ to {:02x}", hsync_pulse_width_, vsync_pulse_time_);
      break;
    case REG_VERT_TOTAL:vert_total_ = data;
      spdlog::info("CRTC: Set vert_total to {:02x}", data);
      break;
    case REG_VERT_TOTAL_ADJ:vert_total_adj_ = data;
      spdlog::info("CRTC: Set vert_total_adj to {:02x}", data);
      break;
    case REG_VERT_TOTAL_DISP:vert_total_disp_ = data;
      spdlog::info("CRTC: Set vert_total_disp to {:02x}", data);
      break;
    case REG_VSYNC_POS:vsync_pos_ = data;
      spdlog::info("CRTC: Set vsync_pos to {:02x}", data);
      break;
    case REG_ILD: {
      ild_ = data & 0x3f;
      spdlog::info("CRTC: Wrote {:02x} to {} register.", data, register_name_[reg_select_]);
      switch (ild_ & 0x03) {
        case 0:
        case 1:spdlog::info("      Normal (non-interlaced) sync mode.");
          break;
        case 2:spdlog::info("      Interlace sync.");
          break;
        case 3:spdlog::info("      Interlace sync and video.");
          break;
      }
      switch ((ild_ >> 2) & 0x03) {
        case 0:spdlog::info("      No display blanking delay.");
          break;
        case 1:spdlog::info("      One character display blanking delay.");
          break;
        case 2:spdlog::info("      Two character display blanking delay.");
          break;
        case 3:spdlog::info("      Video output disabled.");
          break;
      }
      switch ((ild_ >> 4) & 0x03) {
        case 0:spdlog::info("      No cursor blanking delay.");
          break;
        case 1:spdlog::info("      One character cursor blanking delay.");
          break;
        case 2:spdlog::info("      Two character cursor blanking delay.");
          break;
        case 3:spdlog::info("      Cursor output disabled.");
          break;
      }
    }
      break;
    case REG_CHAR_SCAN_LINES:char_scan_lines_ = data & 0x1f;
      spdlog::info("CRTC: Set char_scan_lines to {:02x}", char_scan_lines_);
      break;

    case REG_CURSOR_START:cursor_blink_ = (data >> 6) * 0x01;
      cursor_blink_rate_ = (data >> 5) * 0x01;
      cursor_start_line_ = data & 0x1f;
      spdlog::info("CRTC: Set cursor blink {} blink rate {} start line {}.", cursor_blink_ ? "enabled" : "disabled",
                   cursor_blink_rate_ ? "fast" : "slow",
                   cursor_start_line_);
      break;

    case REG_CURSOR_END:cursor_end_line_ = data & 0x1f;
      spdlog::info("CRTC: Set cursor_end_line to {:02x}", cursor_end_line_);
      break;

    case REG_SCREEN_ADDR_HI:screen_start_ = (screen_start_ & 0xff) | ((data & 0x3f) << 8);
      spdlog::info("CRTC: Screen start addr (hi) set to {:02x}. Screen start at {:04x}", data, screen_start_);
      break;

    case REG_SCREEN_ADDR_LO:screen_start_ = (screen_start_ & 0x3f00) | data;
      spdlog::info("CRTC: Screen start addr (lo) set to {:02x}. Screen start at {:04x}", data, screen_start_);
      break;

    case REG_CURSOR_POS_HI:cursor_pos_ = (cursor_pos_ & 0xff) | ((data & 0x3f) << 8);
      spdlog::info("CRTC: Cursor position (hi) set. Cursor at {} {}",
                   ((cursor_pos_ >> 7) & 0x7f),
                   (cursor_pos_ & 0x7f));
      break;
    case REG_CURSOR_POS_LO:cursor_pos_ = (cursor_pos_ & 0x3f00) | data;
      spdlog::info("CRTC: Cursor position (lo) set. Cursor at {} {}",
                   ((cursor_pos_ >> 7) & 0x7f),
                   (cursor_pos_ & 0x7f));
      break;
    default:
      spdlog::error("CRTC: Attempted to write to illegal register {} {}",
                    reg_select_,
                    reg_select_ <= 17 ? register_name_[reg_select_] : "???");
      break;
  }
}

void Crtc::tick(Bus &bus) {
  auto addr = bus.get_address();
  if (addr < base_addr_ || addr > base_addr_ + CRTC_READ_WRITE) return;
  addr -= base_addr_;

  if (bus.tst_RW()) {
    mmio_read(addr, bus);
  } else {
    mmio_write(addr, bus);
  }
}
