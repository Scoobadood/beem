#include "test_mode_0.h"
#include <spdlog/spdlog-inl.h>

const uint16_t CRTC_BASE = 0xfe00;
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

void tick(Bus &bus, Crtc *crtc) {
  crtc->tick(bus);
  crtc->tick(bus);
  crtc->tick(bus);
  crtc->tick(bus);
}

void set_crtc(Crtc *crtc, Bus &bus, uint8_t reg, uint8_t value) {
  bus.set_address(CRTC_BASE);
  bus.set_data(reg);
  bus.clr_RW();
  tick(bus, crtc);

  bus.set_address(CRTC_BASE + 1);
  bus.set_data(value);
  bus.clr_RW();
  tick(bus, crtc);

  bus.set_address(0);
  bus.clr_RW();
}

void TestMode0::SetUp() {
  bus = Bus();
  crtc = new Crtc(CRTC_BASE);
  set_crtc(crtc, bus, REG_CURSOR_END, 0x08);
  set_crtc(crtc, bus, REG_CURSOR_START, 0x67);
  set_crtc(crtc, bus, REG_CHAR_SCAN_LINES, 0x07);
  set_crtc(crtc, bus, REG_ILD, 0x01);
  set_crtc(crtc, bus, REG_VSYNC_POS, 0x22);
  set_crtc(crtc, bus, REG_VERT_TOTAL_DISP, 0x20);
  set_crtc(crtc, bus, REG_VERT_TOTAL_ADJ, 0x00);
  set_crtc(crtc, bus, REG_VERT_TOTAL, 0x26);
  set_crtc(crtc, bus, REG_SYNCS, 0x82);
  set_crtc(crtc, bus, REG_HSYNC_POS, 0x62);
  set_crtc(crtc, bus, REG_HORZ_DISP, 0x50);
  set_crtc(crtc, bus, REG_HORZ_TOTAL, 0x7f);

  // Screen start addr
  set_crtc(crtc, bus, REG_CURSOR_POS_HI, 0x30);
  set_crtc(crtc, bus, REG_CURSOR_POS_LO, 0x30);

  // Cursor pos
  set_crtc(crtc, bus, REG_SCREEN_ADDR_HI, 0x06); // Actual address / 8
  set_crtc(crtc, bus, REG_SCREEN_ADDR_LO, 0x00);
}

void TestMode0::TearDown() {
  delete crtc;
}

TEST_F(TestMode0, Mode0) {
  crtc->sync();
  for (int t = 0; t < 8000; ++t) {

    auto addr = bus.get_address();
    spdlog::info("{:04} | ADDR: {:04x}    MA: {:04x}    RA:{:02x}    DE: {}    HS:{}    VS:{}",
                 t,
                 addr,
                 crtc->get_ma(),
                 crtc->get_ra(),
                 crtc->display_enable() ? "true " : "false",
                 crtc->hsync() ? "x" : " ",
                 crtc->vsync() ? "x" : " ");

    tick(bus, crtc);
  }
}