
#include "6845_crtc.h"
#include "5c094_vula.h"
#include "bus.h"
#include "screen_data.h"
#include <spdlog/spdlog-inl.h>

#include <fstream>

const uint16_t VULA_BASE = 0xfe20;
const uint16_t CRTC_BASE = 0xfe00;

uint8_t g_tick_count = 0;

void tick(Crtc *crtc, VideoUla *v_ula, Bus &bus) {
  // CRTC runs at_bus 1MHz in this example
  if (g_tick_count % 16 == 0 && crtc)
    crtc->tick(bus);

  // Bus runs at_bus 4MHz
  if (g_tick_count % 4 == 0) {
    auto addr = bus.get_address();
    if (addr >= 0x5800 && addr <= 0x8000) {
      auto data = scr_data[addr - 0x5800];
      bus.set_data(data);
    }
  }

  // Every cycle
  if (v_ula)
    v_ula->tick(bus);

  spdlog::info("{:02}|addr: {:04x}  d8: {:02}", g_tick_count, bus.get_address(), bus.get_data());
  g_tick_count = (g_tick_count + 1) % 16;
}

void tick16(Bus &bus, VideoUla *v_ula, Crtc *crtc) {
  for (auto i = 0; i < 16; ++i) tick(crtc, v_ula, bus);
}

/* Let all 1MHz device run */
void set_vula(VideoUla *vula, Bus &bus, uint8_t reg, uint8_t value) {
  bus.set_address(VULA_BASE + reg);
  bus.set_data(value);
  bus.clr_RW();
  tick16(bus, vula, nullptr);
}

void set_crtc(Crtc *crtc, Bus &bus, uint8_t reg, uint8_t value) {
  bus.set_address(CRTC_BASE);
  bus.set_data(reg);
  bus.clr_RW();
  tick16(bus, nullptr, crtc);

  bus.set_address(CRTC_BASE + 1);
  bus.set_data(value);
  bus.clr_RW();
  tick16(bus, nullptr, crtc);
}

int main(int argc, const char *argv[]) {
  auto v_ula = new VideoUla(VULA_BASE);
  auto crtc = new Crtc(CRTC_BASE);
  Bus bus;

  // Setup the vULA
  set_vula(v_ula, bus, 0x00, 0x88);

  // Set the CRTC (Mode 4)
  set_crtc(crtc, bus, 0x0b, 0x08);
  set_crtc(crtc, bus, 0x0a, 0x67);
  set_crtc(crtc, bus, 0x09, 0x07);
  set_crtc(crtc, bus, 0x08, 0x01);
  set_crtc(crtc, bus, 0x07, 0x22);
  set_crtc(crtc, bus, 0x06, 0x20);
  set_crtc(crtc, bus, 0x05, 0x00);
  set_crtc(crtc, bus, 0x04, 0x26);
  set_crtc(crtc, bus, 0x03, 0x04);
  set_crtc(crtc, bus, 0x02, 0x31);
  set_crtc(crtc, bus, 0x01, 0x28);
  set_crtc(crtc, bus, 0x00, 0x3f);

  // Screen start addr
  set_crtc(crtc, bus, 0x0c, 0x0b);
  set_crtc(crtc, bus, 0x0d, 0x00);

  // Cursor pos
  set_crtc(crtc, bus, 0x0e, 0x0b);
  set_crtc(crtc, bus, 0x0f, 0x00);

  crtc->hw_scroll_addr()->set_data(4, true);
  crtc->hw_scroll_addr()->set_data(5, true);

  uint8_t image[640 * 256 * 3];
  v_ula->reset_clk();
  crtc->sync();

  do {
    tick(crtc, v_ula, bus);
  } while (bus.get_address() < 0x5862);
  std::ofstream f{"/Users/dave/Desktop/img.ppm"};
  f << "P3" << std::endl;
  f << "320 256" << std::endl;
  f << "255" << std::endl;
  for (int i = 0; i < 10240; i += 3) {
    f << std::to_string(image[i]) << " " << std::to_string(image[i + 1]) << " " << std::to_string(image[i + 2])
      << std::endl;
  }
  f.close();
}