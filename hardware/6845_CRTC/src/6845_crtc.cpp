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
        : base_addr_{base_addr} //
        , reg_select_{0}//
        , horz_total_{0} //
        , horz_disp_{0} //
        , horz_sync_pos_{0} //
        , vert_total_{0} //
        , vert_total_adj_{0} //
        , vert_disp_{0} //
        , horz_sync_width_{0} //
        , vert_sync_width_{0} //
        , vert_sync_pos_{0} //
        , ilace_skew_{0} //
        , max_raster_lines_{0} //
        , curs_blink_{0} //
        , curs_blink_rate_{0} //
        , curs_start_raster_{0} //
        , curs_end_raster_{0} //
        , scr_start_addr_{0} //
        , curs_start_addr_{0} //
        , light_pen_pos_{0} //
        , char_cnt_{0} //
        , line_cnt_{0} //
        , raster_cnt_{0} //
        , adj_cnt_{0} //
        , hsync_width_cnt_{0} //
        , vsync_width_cnt_{0} //
        , cursor_enabled_{false} //
        , memory_addr_{0} //
        , h_disp_enable_{0} //
        , v_disp_enable_{0} //
        , hsync_{0} //
        , vsync_{0} //
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
}

void Crtc::mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  if (addr == CRTC_REG_SELECT) {
    spdlog::error("CRTC: Invalid attempt to read from REG_SELECT");
    return;
  }

  auto data = bus->get_data();
  switch (reg_select_) {
    case REG_CURSOR_POS_HI:
      data = (curs_start_addr_ >> 8) & 0x3f;
      spdlog::get("CRTC")->info("CRTC: Read cursor position hi ({:02x})", data);
      break;
    case REG_CURSOR_POS_LO:
      data = curs_start_addr_ & 0xff;
      spdlog::get("CRTC")->info("CRTC: Read cursor position lo ({:02x})", data);
      break;
    case REG_LPEN_POS_HI:
      data = (light_pen_pos_ >> 8) & 0x3f;
      spdlog::get("CRTC")->info("CRTC: Read light pen position hi ({:02x})", data);
      break;
    case REG_LPEN_POS_LO:
      data = curs_start_addr_ & 0xff;
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
      horz_total_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to horz_total", data);
      break;

    case REG_HORZ_DISP:
      horz_disp_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to horz_disp", data);
      break;

    case REG_HSYNC_POS:
      horz_sync_pos_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to hsync_pos", data);
      break;

    case REG_SYNCS: {
      auto hw = data & 0xf;
      auto vw = (data >> 4) & 0xf;
      vert_sync_width_ = (vw == 0) ? 16 : vw;
      if (hw != 0) horz_sync_width_ = hw;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to reg_syncs.", data);
      spdlog::get("CRTC")->info("      hsync_pulse is {:02x} {}", horz_sync_width_, (hw == 0) ? "[ignored 0]" : "");
      spdlog::get("CRTC")->info("      vsync_time_ is {:02x}", vert_sync_width_);
    }
      break;

    case REG_VERT_TOTAL:
      vert_total_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to vert_total", data);
      break;

    case REG_VERT_TOTAL_ADJ:
      vert_total_adj_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to vert_total_adj", data);
      break;

    case REG_VERT_TOTAL_DISP:
      vert_disp_ = data;
      sync();
      spdlog::get("CRTC")->info("CRTC: {:02x} to vert_total_disp", data);
      break;

    case REG_VSYNC_POS:
      vert_sync_pos_ = data;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to vsync_pos", data);
      break;

    case REG_ILD: {
      ilace_skew_ = data & 0x3f;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to ILD.", data);
      switch (ilace_skew_ & 0x03) {
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
      switch ((ilace_skew_ >> 2) & 0x03) {
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
      switch ((ilace_skew_ >> 4) & 0x03) {
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
      max_raster_lines_ = data & 0x1f;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to char_scan_lines. Scan lines set to {:02x}", data,
                                max_raster_lines_);
      break;

    case REG_CURSOR_START:
      curs_blink_ = (data >> 6) & 0x01;
      curs_blink_rate_ = (data >> 5) & 0x01;
      curs_start_raster_ = data & 0x1f;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cursor_start_reg.", data);
      spdlog::get("CRTC")->info("      Cursor blink {}", curs_blink_ ? "enabled" : "disabled");
      spdlog::get("CRTC")->info("      blink rate {}", curs_blink_rate_ ? "fast" : "slow");
      spdlog::get("CRTC")->info("      start line {}.", curs_start_raster_);
      break;

    case REG_CURSOR_END:
      curs_end_raster_ = data & 0x1f;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cursor_end_line", curs_end_raster_);
      break;

    case REG_SCREEN_ADDR_HI:
      scr_start_addr_ = (scr_start_addr_ & 0x00ff) | ((data & 0x3f) << 8);
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to scr_start_addr_hi.", data);
      spdlog::get("CRTC")->info("      Screen start address is {:04x}", scr_start_addr_);
      break;

    case REG_SCREEN_ADDR_LO:
      scr_start_addr_ = (scr_start_addr_ & 0x3f00) | data;
      sync();
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to scr_start_addr_lo.", data);
      spdlog::get("CRTC")->info("      Screen start address is {:04x}", scr_start_addr_);
      break;

    case REG_CURSOR_POS_HI:
      curs_start_addr_ = (curs_start_addr_ & 0xff) | ((data & 0x3f) << 8);
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cur_pos_hi.", data);
      spdlog::get("CRTC")->info("      Cursor pos is x{:04x} : {},{}",
                                curs_start_addr_,
                                ((curs_start_addr_ >> 7) & 0x7f),
                                (curs_start_addr_ & 0x7f));
      break;

    case REG_CURSOR_POS_LO:
      curs_start_addr_ = (curs_start_addr_ & 0x3f00) | data;
      spdlog::get("CRTC")->info("CRTC: Wrote {:02x} to cur_pos_lo.", data);
      spdlog::get("CRTC")->info("      Cursor pos is x{:04x} : {},{}",
                                curs_start_addr_,
                                ((curs_start_addr_ >> 7) & 0x7f),
                                (curs_start_addr_ & 0x7f));
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

void Crtc::generate_next_address(const std::shared_ptr<Bus> &dram_bus) {
  if (char_cnt_ == horz_total_) {
    char_cnt_ = 0;
    if (raster_cnt_ == max_raster_lines_) {
      raster_cnt_ = 0;
    }
    if (line_cnt_ == vert_total_ && adj_cnt_ == vert_total_adj_) {
      line_cnt_ = 0;
      adj_cnt_ = 0;
      h_disp_enable_ = 1;
      v_disp_enable_ = 1;
    }
    memory_addr_ = line_cnt_ * horz_disp_ + scr_start_addr_;
  } else {
    memory_addr_++;
  }
  if (frame_cnt_ == 64) frame_cnt_ = 0;
  uint16_t output_addr = ((raster_cnt_) & 0x07) | (memory_addr_ << 3);


  char_cnt_++;
  if (char_cnt_ == horz_disp_) {
    h_disp_enable_ = 0;
  }
  if (hsync_) {
    hsync_width_cnt_++;
    if (hsync_width_cnt_ == horz_sync_width_) {
      hsync_ = 0;
    }
  }
  if (char_cnt_ == horz_sync_pos_) {
    hsync_ = 1;
    hsync_width_cnt_ = 0;
  }
  bool raster_ended = false;
  if (char_cnt_ == horz_total_) {
    raster_cnt_++;
    raster_ended = true;
    h_disp_enable_ = 1;
  }

  bool line_ended = false;
  if (raster_ended) {
    if (raster_cnt_ == max_raster_lines_) {
      line_cnt_++;
      line_ended = true;
    }
  }

  if (line_ended) {
    if (line_cnt_ == vert_disp_) {
      v_disp_enable_ = 0;
      ++frame_cnt_;
    }

    if (vsync_) {
      vsync_width_cnt_++;
      if (vsync_width_cnt_ == vert_sync_width_) {
        vsync_ = 0;
      }
    }

    if (line_cnt_ == vert_sync_pos_) {
      vsync_ = 1;
      vsync_width_cnt_ = 0;
    }
  }

  if ((line_cnt_ == vert_total_) && raster_ended) {
    if (adj_cnt_ != vert_total_adj_) {
      ++adj_cnt_;
    }
  }


  /*
   * Cursor handling extrapolated from here:
   * https://www.cpcwiki.eu/index.php/VHDL_implementation_of_the_6845
   * if RA >= CURST(4 downto 0) and RA <= CUREND and ACTIVE = '1' then
		case CURST(6 downto 5) is
			when "00" =>
				CURSOR <= '1';
			when "10" =>
				CURSOR <= CTR_BLINK(3);
			when "11" =>
				CURSOR <= CTR_BLINK(4);
			when others =>
				CURSOR <= '0';
		end case;		
	else
		CURSOR <= '0';
	end if;
   */
  // Active?
  cursor_enabled_ = false;
  if (memory_addr_ == curs_start_addr_) {
    if (raster_cnt_ >= curs_start_raster_ && raster_cnt_ <= curs_end_raster_) {
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

  // TODO: Fix wraparound per description here: https://beebwiki.mdfs.net/Address_translation
  if (output_addr & 0x8000) {
// Decode latch_ bits.
    uint8_t c0c1 = hw_scroll_addr_->data();
    switch (c0c1) {
      case 0x00:
        output_addr -= 0x4000;
        break;
      case 0x10:
        output_addr -= 0x2000;
        break;
      case 0x20:
        output_addr -= 0x5000;
        break;
      case 0x30:
        output_addr -= 0x2800;
        break;
      default:
        spdlog::get("CRTC")->error("Latch bits for base addr have crazy value ({:02x})", c0c1);
        spdlog::error("Latch bits for base addr have crazy value ({:02x})", c0c1);
        break;
    }
  }
  if (output_addr >= 0x8000) {
    spdlog::get("CRTC")->error("About to output an uncorrected video address:  ({:04x})", output_addr);
    spdlog::error("About to output an uncorrected video address:  ({:04x})", output_addr);
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

/* Force screen paint from top of screen */
void Crtc::sync() {
  memory_addr_ = scr_start_addr_;
  char_cnt_ = horz_total_;
  raster_cnt_ = vert_total_adj_;
  line_cnt_ = vert_total_;
  adj_cnt_ = vert_total_adj_;
  vsync_ = 0;
  hsync_ = 0;
  v_disp_enable_ = 1;
  h_disp_enable_ = 1;
}
