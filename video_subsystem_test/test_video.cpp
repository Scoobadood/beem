
#include "6845_crtc.h"
#include "5c095_vula.h"
#include "bus.h"

const uint16_t VULA_BASE = 0xfe20;
const uint16_t CRTC_BASE = 0xfe00;

void set_vula(VideoUla *vula, Bus &bus, uint8_t reg, uint8_t value) {
  bus.set_address(VULA_BASE + reg);
  bus.set_data(value);
  bus.clr_RW();
  vula->tick(bus);
}

void set_crtc(Crtc *crtc, Bus &bus, uint8_t reg, uint8_t value) {
  bus.set_address(CRTC_BASE);
  bus.set_data(reg);
  bus.clr_RW();
  crtc->tick(bus);

  bus.set_address(CRTC_BASE + 1);
  bus.set_data(value);
  bus.clr_RW();
  crtc->tick(bus);
}

int main(int argc, const char *argv[]) {
  auto v_ula = new VideoUla(VULA_BASE);
  auto crtc = new Crtc(CRTC_BASE);
  Bus bus;

  // Setup the vULA
  set_vula(v_ula, bus, 0x00, 0x4b);

  // Set the CRTC
  set_crtc(crtc, bus, 0x0b, 0x13);
  set_crtc(crtc, bus, 0x0a, 0x72);
  set_crtc(crtc, bus, 0x09, 0x12);
  set_crtc(crtc, bus, 0x08, 0x93);
  set_crtc(crtc, bus, 0x07, 0x1c);
  set_crtc(crtc, bus, 0x06, 0x19);
  set_crtc(crtc, bus, 0x05, 0x02);
  set_crtc(crtc, bus, 0x04, 0x1e);
  set_crtc(crtc, bus, 0x03, 0x24);
  set_crtc(crtc, bus, 0x02, 0x33);
  set_crtc(crtc, bus, 0x01, 0x28);
  set_crtc(crtc, bus, 0x00, 0x3f);

  // Screen start addr
  set_crtc(crtc, bus, 0x0c, 0x28);
  set_crtc(crtc, bus, 0x0d, 0x00);

  // Cursor pos
  set_crtc(crtc, bus, 0x0e, 0x28);
  set_crtc(crtc, bus, 0x0f, 0x00);

  while (true) {
    crtc->tick(bus);
    v_ula->tick(bus);
  }
}