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
    , tick_count_{0} //
    , crtc_{nullptr} //
{
  try {
    auto logger = spdlog::basic_logger_mt("vULA", "logs/vULA.txt", true);
    spdlog::flush_every((std::chrono::seconds) 5);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
}

void VideoUla::write_palette(uint8_t data) {
  auto logical = (data >> 4) & 0xf;
  auto actual = data & 0xf;
  palette_[logical] = actual;
  spdlog::get("vULA")->info("vULA: Set palette logical {:02x} to actual {}", logical, colour_name_[actual]);
}

void VideoUla::mmio_write(uint16_t addr, Bus &bus) {
  auto data = bus.get_data();
  switch (addr - base_addr_) {
    case VCR_WO: {
      vula_ctl_ = data;
      crtc_clk_ = (data & 0x10) >> 4;
      auto cwb = ((data >> 5) & 0x3);
      switch (cwb) {
        case 0: cursor_width_ = 1;
          break;
        case 2: cursor_width_ = 2;
          break;
        case 3: cursor_width_ = 4;
          break;
        default:spdlog::error("vULA: Bad cursor width {} in {:02x}", cwb, data);
          cursor_width_ = 1;
          break;
      }
      spdlog::get("vULA")->info("vULA: Writing {:02x} to VULA_CTL", bus.get_data());
      spdlog::get("vULA")->info("      Flash colour {}", data & 0x01);
      spdlog::get("vULA")->info("      Teletext mode? {}", data & 0x02 ? "Yes" : "No");
      spdlog::get("vULA")->info("      {} characters per line", ((0x01 << ((data >> 2) & 0x03)) * 10));
      spdlog::get("vULA")->info("      CRTC clk freq {}MHz", crtc_clk_ ? 2 : 1);
      spdlog::get("vULA")->info("      Cursor width in bytes {}", cursor_width_);
      spdlog::get("vULA")->info("      Master cursor width {}", (data & 0x80) ? "large" : "small");

      // Shift clock is set  baed on number of characters on screen
      shift_clk_ = 0x01 << (((data >> 2) & 0x03) + 1);
      break;
    }

    case PALETTE_WO:write_palette(data);
      break;

    default:spdlog::warn("vULA: Unimplemented write of {:02x} to {:04x}", bus.get_data(), addr);
      break;
  }
}

bool VideoUla::time_to_shift() {
  return (--shift_countdown_ == 0);
}

void VideoUla::process_data() {
  uint8_t logical_colour =
      ((curr_data_ & 0x80) >> 4) |
          ((curr_data_ & 0x20) >> 3) |
          ((curr_data_ & 0x08) >> 2) |
          ((curr_data_ & 0x02) >> 1);
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
  spdlog::get("vULA")->info("{}| Logical colour {}, actual {}", tick_count_, logical_colour, actual_colour);
  if (time_to_shift()) {
    curr_data_ = ((curr_data_ << 1) & 0xfe) | 0x01;
    reset_shift_clk();
    spdlog::get("vULA")->info("{}| shift", tick_count_);
  }
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
}

void VideoUla::maybe_poll_crtc(Bus &bus) {
  if ((tick_count_ == 0) || (tick_count_ == 8 && crtc_clk_ == 1)) {
    spdlog::get("vULA")->info("{}| reload ({:02x}) from &{:04x}", tick_count_,
                              bus.get_data(), bus.get_address());

    if (crtc_)
      crtc_->tick(bus);
    curr_data_ = bus.get_data();
    reset_shift_clk();
  }
}

void VideoUla::set_crtc(Crtc *crtc) {
  crtc_ = crtc;
}

void VideoUla::tick(Bus &bus) {
  auto addr = bus.get_address();
  if (addr == base_addr_ || addr == base_addr_ + 1) {
    if (bus.tst_RW()) {
      spdlog::error("vULA: Unsupported attempt to read Video ULA read from {:04x}", addr);
    } else {
      mmio_write(addr, bus);
    }
    tick_count_ = (tick_count_ + 1) % 16;
    return;
  }

  maybe_poll_crtc(bus);
  process_data();
  tick_count_ = (tick_count_ + 1) % 16;
}