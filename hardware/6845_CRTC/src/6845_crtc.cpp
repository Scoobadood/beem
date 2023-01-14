/*
 * https://en.wikipedia.org/wiki/Motorola_6845
 */
#include "6845_crtc.h"
#include <spdlog/spdlog-inl.h>

const uint8_t REG_SELECT = 0x00;
const uint8_t READ_WRITE = 0x01;

Crtc::Crtc(uint16_t base_addr) //
    : base_addr_{base_addr} //
    , reg_select_{0}//
    , register_name_
          {
              "Horizontal total", "Horizontal displayed characters" "Horizontal sync position",
              "Horizontal sync width/Vertical sync time", "Vertical total", "Vertical total adjust",
              "Vertical displayed characters", "Vertical sync position", "Interlace/Display delay/Cursor delay",
              "Scan lines per character", "Cursor start line and blink type", "Cursor end line", "Screen start address",
              "Screen start address", "Cursor position", "Cursor position", "Light pen position", "Light pen position",
              "Cursor width (BBFW)"
          }//
{}

void Crtc::mmio_read(uint16_t addr, Bus &bus) {
  if (addr == REG_SELECT) {
    spdlog::error("CRTC: Invalid attempt to read from REG_SELECT");
    return;
  }

  if (reg_select_ >= 0 && reg_select_ <= 13) {
    spdlog::error("CRTC: Attempt to read from WO register {}", reg_select_);
    return;
  }
  // TODO: Handle read
}

void Crtc::mmio_write(uint16_t addr, Bus &bus) {
  if (addr == REG_SELECT) {
    auto reg = bus.get_data();
    if (reg > 17) {
      spdlog::error("CRTC: Selected invalid register {}", reg);
    } else {
      spdlog::info("CRTC: Selected {} register", register_name_[reg]);
    }
    reg_select_ = reg & 0x1f;
    return;
  }

  if (reg_select_ == 16 || reg_select_ == 17) {
    spdlog::error("CRTC: Attempt to write to RO register {}", reg_select_);
    return;
  }
  // TODO: Handle write
  auto data = bus.get_data();
  spdlog::info("CRTC: Writing {:02x} to {} register", data, register_name_[reg_select_]);
}

void Crtc::tick(Bus &bus) {
  auto addr = bus.get_address();
  if (addr < base_addr_ || addr > base_addr_ + READ_WRITE) return;
  addr -= base_addr_;

  if (bus.tst_RW()) {
    mmio_read(addr, bus);
  } else {
    mmio_write(addr, bus);
  }
}
