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
{}

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
      spdlog::info("vULA: Writing {:02x} to VULA_CTL", bus.get_data());
      spdlog::info("      Flash colour {}", data & 0x01);
      spdlog::info("      Teletext mode? {}", data & 0x02 ? "Yes" : "No");
      spdlog::info("      {} characters per line", ((0x01 << ((data >> 2) & 0x03)) * 10));
      spdlog::info("      {} frequency clock", (data & 0x10) ? "High" : "Low");
      auto cwb = ((data >> 5) & 0x3);
      spdlog::info("      Cursor width in bytes {}", ((cwb == 3) ? "4" : (cwb == 2) ? "2" : (cwb == 0) ? "1" : "?"));
      spdlog::info("      Master cursor width {}", (data & 0x80) ? "large" : "small");
      break;
    }

    case PALETTE_WO:write_palette(data);
      break;

    default:spdlog::warn("vULA: Unimplemented write of {:02x} to {:04x}", bus.get_data(), addr);
      break;
  }
}

void VideoUla::tick(Bus &bus) {
  auto addr = bus.get_address();
  if (addr < base_addr_ || addr > base_addr_ + 1) return;
  if (bus.tst_RW()) {
    spdlog::error("vULA: Unsupported attempt to read Video ULA read from {:04x}", addr);
    return;
  }
  mmio_write(addr, bus);
}