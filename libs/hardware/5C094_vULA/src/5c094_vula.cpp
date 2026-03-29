#include "5c094_vula.h"
#include "6845_crtc.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

const uint8_t VCR_WO = 0x00;
const uint8_t PALETTE_WO = 0x01;

VideoUla::VideoUla(uint16_t base_addr) //
    : base_addr_{base_addr} //
    , palette_{0, 1, 2, 3, 4, //
               5, 6, 7, 8, 9, //
               10, 11, 12, 13, 14, 15}//
    , colour_name_{"black", "red", "green", "yellow",//
                   "blue", "magenta", "cyan", "white",//
                   "flashing black–white", "flashing red–cyan",//
                   "flashing green–magenta", "flashing yellow–blue",//
                   "flashing blue–yellow", "flashing magenta–green", //
                   "flashing cyan–red", "flashing white–black"} //
    , vula_ctl_{0}//
    , red_{0} //
    , grn_{0} //
    , blu_{0} //
    , shift_clk_{0} //
    , shift_countdown_{0} //
    , crtc_clk_{0} //
    , cursor_width_{0} //
    , curr_data_{0} //
    , last_latched_addr_{0} //
    , last_latched_data_{0}//
    , num_shifts_{0}//
    , crtc_{nullptr} //
{
  try {
    auto logger = spdlog::basic_logger_mt("vULA", "logs/vULA.txt", true);
    logger->flush_on(spdlog::level::err);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
  logger_ = spdlog::get("vULA");
  bus_dance_logger_ = spdlog::get("BusDance");
}

/**
 * Update ULA palette data
 * data consists of a four bit logical colour in bits 4-7 and an
 * actual colour in bits 0-3
 */
void VideoUla::write_palette(uint8_t data) {
  auto logical = (data >> 4) & 0xf;
  auto actual = data & 0xf;
  palette_[logical] = actual;
  logger_->info("vULA: Set palette logical {:02x} to actual {}", logical, colour_name_[actual]);
}

/*
 * Write to ULA registers.
 *
 */
void VideoUla::mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  auto data = bus->get_data();
  switch (addr - base_addr_) {
    case VCR_WO: {
      vula_ctl_ = data;
      crtc_clk_ = (data & 0x10) >> 4;
      auto cwb = ((data >> 5) & 0x3);
      switch (cwb) {
        case 0:
          cursor_width_ = 1;
          break;
        case 2:
          cursor_width_ = 2;
          break;
        case 3:
          cursor_width_ = 4;
          break;
        default:
          spdlog::error("vULA: Bad cursor width {} in {:02x}", cwb, data);
          cursor_width_ = 1;
          break;
      }
      auto num_cols_flags = (data >> 2) & 0x03;
      /*
       * This is displayed characters, not CRTC characters.
       * b3	b2	Number of columns	 Pixel rate   Mode
       * 0	0	     10	              2 MHz
       * 0	1	     20	              4 MHz       2, 5
       * 1	0	     40	              8 MHz       1, 4, 6, 7
       * 1	1	     80	             16 MHz       0, 3
       */
      auto chars_per_line = (1 << num_cols_flags) * 10;
      shift_clk_ = (1 << num_cols_flags) * 2;
      // Sets up the shift countdown
      reset_shift_clk();
      logger_->info("vULA: Writing {:02x} to VULA_CTL", bus->get_data());
      logger_->info("      Flash colour {}", data & 0x01);
      logger_->info("      Teletext mode? {}", data & 0x02 ? "Yes" : "No");
      logger_->info("      {} characters per line", chars_per_line);
      logger_->info("      CRTC clk freq {}MHz", crtc_clk_ ? 2 : 1);
      logger_->info("      Cursor width in bytes {}", cursor_width_);
      logger_->info("      Master cursor width {}", (data & 0x80) ? "large" : "small");
      logger_->info("      Shift clock {}MHz", shift_clk_);
      break;
    }

    case PALETTE_WO:
      write_palette(data);
      break;

    default:
      spdlog::warn("vULA: Unimplemented write of {:02x} to {:04x}", bus->get_data(), addr);
      break;
  }
}

bool VideoUla::time_to_shift() {
  return (--shift_countdown_ == 0);
}

void VideoUla::process_data() {
  uint8_t logical_colour =
      ((curr_data_ & 0x80) >> 4) | ((curr_data_ & 0x20) >> 3) |
          ((curr_data_ & 0x08) >> 2) | ((curr_data_ & 0x02) >> 1);
  auto actual_colour = palette_[logical_colour];
  red_ = 255 * (actual_colour & 0x01);
  grn_ = 255 * ((actual_colour & 0x02) >> 1);
  blu_ = 255 * ((actual_colour & 0x04) >> 2);
  bool flash = (actual_colour & 0x08);
  if (!(flash && (vula_ctl_ & 0x01))) {
    red_ = 255 - red_;
    grn_ = 255 - grn_;
    blu_ = 255 - blu_;
  }
  if (crtc_->cursor_enabled()) {
    red_ = 255 - red_;
    grn_ = 255 - grn_;
    blu_ = 255 - blu_;
  }
  // trace: process_data logical_colour, actual_colour, red_, grn_, blu_
}

void VideoUla::reset_shift_clk() {
  switch (shift_clk_) {
    case 16:
      shift_countdown_ = 1;
      break;
    case 8:
      shift_countdown_ = 2;
      break;
    case 4:
      shift_countdown_ = 4;
      break;
    case 2:
      shift_countdown_ = 8;
      break;
  }
  // trace: reset shift clock to shift_countdown_
}

void VideoUla::maybe_drive_crtc(const std::shared_ptr<Bus> &dram_bus) {
  if (crtc_) {
    // Address generation on two cycles or one depending on clock frequency
    if (clock_->went_low(CLK_1_MHZ) || ((crtc_clk_ == 1) && clock_->went_low(CLK_2_MHZ))) {
      crtc_->generate_next_address(dram_bus);
      latch_new_data(dram_bus);
    }
  }
}

/**
 * Latch data from DRAM when clock goes high. This is to give the CRTC a chance to write data
 * @param dram_bus
 */
void VideoUla::latch_new_data(const std::shared_ptr<Bus> &dram_bus) {
  bus_dance_logger_->debug("VULA: Latching data from CRTC from DRAM bus {:04x} {:02x} {} {}",
                                 dram_bus->get_address(),
                                 dram_bus->get_data(),
                                 dram_bus->tst_RW() ? "R" : "W",
                                 dram_bus->tst_SYNC() ? "SYN" : "   ",
                                 dram_bus->tst_RST() ? "RST" : "");
  curr_data_ = dram_bus->get_data();
  last_latched_data_ = curr_data_;

  logger_->debug("Latched new data {:02x} from DRAM {:04x}", curr_data_,
                             dram_bus->get_address());

  logger_->trace("Clks: 4Mhz {}, 2MHz {}, 2MHzE {}, 1Mhz {}",
                             clock_->is_high(CLK_4_MHZ) ? "H" : "L",
                             clock_->is_high(CLK_2_MHZ) ? "H" : "L",
                             clock_->is_high(CLK_E_2_MHZ) ? "H" : "L",
                             clock_->is_high(CLK_1_MHZ) ? "H" : "L");

  last_latched_addr_ = dram_bus->get_address();
  if (crtc_->last_generated_address() != last_latched_addr_) {
    spdlog::error("VULA: Latched address does not match last generated address");
  }
  num_shifts_ = 0;
}

void VideoUla::set_crtc(const std::shared_ptr<Crtc> &crtc) {
  crtc_ = crtc;
}

/**
 * ULA ticks on the 16MHz clock
 * @param main_bus
 * @param dram_bus
 */
void VideoUla::tick(const std::shared_ptr<Bus> &main_bus,
                    const std::shared_ptr<Bus> &dram_bus) {

  auto addr = main_bus->get_address();
  if (addr == base_addr_ || addr == base_addr_ + 1) {
    if (main_bus->tst_RW()) {
      spdlog::error("vULA: Unsupported attempt to read Video ULA read from {:04x}", addr);
    } else {
      mmio_write(addr, main_bus);
    }
    return;
  }

  maybe_drive_crtc(dram_bus);

  // Makes RGB values available
  process_data();

  if (time_to_shift()) {
    curr_data_ = ((curr_data_ << 1) & 0xfe) | 0x01;
    num_shifts_++;
    logger_->trace("Shift at clks: 4Mhz {}, 2MHz {}, 2MHzE {}, 1Mhz {}",
                               clock_->is_high(CLK_4_MHZ) ? "H" : "L",
                               clock_->is_high(CLK_2_MHZ) ? "H" : "L",
                               clock_->is_high(CLK_E_2_MHZ) ? "H" : "L",
                               clock_->is_high(CLK_1_MHZ) ? "H" : "L");
    reset_shift_clk();
  }
}

uint32_t VideoUla::rgb() const {
  auto rgb = ((red_ << 16) | (grn_ << 8) | blu_) & 0xffffff;
  return rgb;
}

void VideoUla::dump(std::shared_ptr<spdlog::logger> &logger) {
  logger->debug("vULA: last read: {:04x},   "
                "val: {:02x}, curr: {:02x}, #shift: {}, cntdwn: {}",
                last_latched_addr_,
                last_latched_data_,
                curr_data_,
                num_shifts_,
                shift_countdown_);
}