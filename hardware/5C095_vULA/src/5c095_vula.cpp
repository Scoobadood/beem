#include "5c095_vula.h"

#include <spdlog/spdlog-inl.h>

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
    , tick_count_{0}//
{}

void VideoUla::render_to(uint8_t *buffer, uint32_t buffer_length) {
  buffer_ = buffer;
  buffer_length_ = buffer_length;
  buffer_idx_ = 0;
}

void VideoUla::write_palette(uint8_t data) {
  auto logical = (data >> 4) & 0xf;
  auto actual = data & 0xf;
  palette_[logical] = actual;
  spdlog::info("vULA: Set palette logical {:02x} to actual {}", logical, colour_name_[actual]);
}

void VideoUla::mmio_write(uint16_t addr, Bus &bus) {
  auto data = bus.get_data();
  switch (addr - base_addr_) {
    case VCR_WO: {
      vula_ctl_ = data;
      clk_freq_ = (data & 0x10) ? 16 : 8;
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
      spdlog::info("vULA: Writing {:02x} to VULA_CTL", bus.get_data());
      spdlog::info("      Flash colour {}", data & 0x01);
      spdlog::info("      Teletext mode? {}", data & 0x02 ? "Yes" : "No");
      spdlog::info("      {} characters per line", ((0x01 << ((data >> 2) & 0x03)) * 10));
      spdlog::info("      Clk freq {}MHz", clk_freq_);
      spdlog::info("      Cursor width in bytes {}", cursor_width_);
      spdlog::info("      Master cursor width {}", (data & 0x80) ? "large" : "small");
      break;
    }

    case PALETTE_WO:write_palette(data);
      break;

    default:spdlog::warn("vULA: Unimplemented write of {:02x} to {:04x}", bus.get_data(), addr);
      break;
  }
}

void VideoUla::process_data_to_image(uint8_t data) {
  if (buffer_ == nullptr) return;
  uint8_t logical_colour = ((data & 0x40) >> 6) | ((data & 0x10) >> 4) | ((data & 0x04) >> 2) | (data & 0x01);
  auto actual_colour = palette_[logical_colour];
  uint8_t r = 255 * (actual_colour & 0x01);
  uint8_t g = 255 * (actual_colour & 0x02);
  uint8_t b = 255 * (actual_colour & 0x04);
  bool flash = (actual_colour & 0x08);
  if (!(flash && (vula_ctl_ & 0x01))) {
    r = 255 - r;
    g = 255 - g;
    b = 25 - b;
  }

  //          clk frq  cursw  shft    gfx wid   txt  cpl   cols
  // Mode 0 :  Hi       1      16       640    80x32  80     2
  // Mode 1 :  Hi       2       8       320    40x32  80     4
  // Mode 2 :  Hi       4       4       160    20x32  80     16
  // Mode 3 :  Hi       1      16       xxx    80x25  80     2   text only
  // Mode 4 :  Lo       1       8       320    40x32  40     2
  // Mode 5 :  Lo       2       4       160    20x32  40     4
  // Mode 6 :  Lo       1       8       xxx    40x25  40     2   text only
  // Mode 7*:  Lo       2               xxx    40x25  40     TELETEXT
  // Pixels to render is Clk freq (Hi=16MHz, Lo=8MHz) / cursor_width
  auto pixel_count = clk_freq_ / cursor_width_;
  for (auto i = 0; i < pixel_count; ++i) {
    buffer_[(buffer_idx_ = (buffer_idx_ + 1) % buffer_length_)] = r;
    buffer_[(buffer_idx_ = (buffer_idx_ + 1) % buffer_length_)] = g;
    buffer_[(buffer_idx_ = (buffer_idx_ + 1) % buffer_length_)] = b;
  }
}

void VideoUla::tick(Bus &bus) {
  auto addr = bus.get_address();

  // If this is tick 0, this is a request from the CPU
  if (tick_count_ == 0) {
    if (addr < base_addr_ || addr > base_addr_ + 1) return;
    if (bus.tst_RW()) {
      spdlog::error("vULA: Unsupported attempt to read Video ULA read from {:04x}", addr);
      return;
    }
    mmio_write(addr, bus);
  } else if (tick_count_ == 2) {
    // This is CRTC generated data, we should read a byte from address and turn into image data.
    // addr should be in VRAM
    auto data = bus.get_data();
    process_data_to_image(data);
  }
  tick_count_ = (tick_count_ + 1) % 4;
}