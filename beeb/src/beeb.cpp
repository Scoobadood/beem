#include "beeb.h"
#include "rom.h"
#include "clock.h"

#include  <fstream>
#include  <iostream>

#include <spdlog/spdlog-inl.h>

/* Memory Map constants */
const uint16_t DRAM_BASE = 0x0000;
const uint16_t DRAM_LAST = 0x7fff;
const uint16_t DRAM_SIZE = DRAM_LAST - DRAM_BASE + 1;

const uint16_t BASIC_ROM_BASE = 0x8000;
const uint16_t BASIC_ROM_LAST = 0xBfff;
const uint16_t BASIC_ROM_SIZE = BASIC_ROM_LAST - BASIC_ROM_BASE + 1;
const uint16_t MOS_ROM_BASE = 0xC000;

const uint16_t MMIO_VULA_REG_SEL = 0xfe20;
const uint16_t MMIO_VULA_PLT = 0xfe21;

Beeb::Beeb() {
  using namespace std;

  // Make a common clock for most things
  clock_ = make_shared<Clock>();

  cpu_ = make_shared<M6502>();

  bus_ = make_shared<Bus>();
  dram_bus_ = make_shared<Bus>();
  dram_ = make_shared<DRAM>(DRAM_SIZE, DRAM_BASE);
  basic_rom_ = make_shared<Rom>("data/Basic2.rom", BASIC_ROM_BASE);
  mos_ = make_shared<Rom>("data/os120.bin", MOS_ROM_BASE);


//  system_via_ = new Via(clock_, 0xfe40);
//  user_via_ = new Via(clock_, 0xfe60);
//
//  latch_ = new IC32Latch(clock_);
//
//  system_via_->subscribe_port_b(latch_->src());
//
//  // Attach Sound chip
//  sound_chip_ = new SN76489(clock_);
//  latch_->subscribe(sound_chip_->we_src());
//  system_via_->subscribe_port_a(sound_chip_->data_src());
//
//  // Attach keyboard
//  keyboard_ = new Keyboard(clock_, 0x01 /* Boot into mode 6 */);
//  latch_->subscribe(keyboard_->we_src());
//  latch_->subscribe(keyboard_->cl_led_src());
//  latch_->subscribe(keyboard_->sl_led_src());
//  system_via_->subscribe_port_a(keyboard_->data_src());
//  system_via_->provide_port_a(keyboard_->provider());
//
//  // ACIA
//  acia_ = new Acia(clock_);
//
//  // ADC
//  adc_ = new Adc(clock_, 0xfec0);
//
//
//  // Video ULA
//  v_ula_ = new VideoUla(clock_, 0xfe20);
//
//  // CRTC
//  crtc_ = new Crtc(0xfe00);
//  latch_->subscribe(crtc_->hw_scroll_addr());
//  v_ula_->set_crtc(crtc_);
}

void Beeb::reset() {
  bus_->clr_RST();
  bus_->clr_SYNC();
  cpu_->tick(bus_);
  bus_->set_RST();
  do {
    clock_->tick();
    cpu_->tick(bus_);
    dram_->tick(bus_);
    mos_->tick(bus_);
  } while (!bus_->tst_SYNC());
}

/*
 *               --o~~~~~~o-                            +------+       _______
 *               d3|      |c3                           |      o<----- VIDPROC
 *               --o IC14 o-                         +--o IC21 |
 *               d2|      |c2                        |  | NAND o<----- A15 (ie address 8000+)
 *               --o      o-                     (5) |  +------+
 *               d1|      |c1                        |
 *               --o      o-              +------+   |
 *               d0|      |c0             |      o<--+
 *               --o      o-              |      |
 *              t/r|      |enable    (3)  |      o<-- +5V
 * 6502 R/w ------>o      o<--------------o IC25 |
 *                 +------+               | NAND o<-- phi in (2MHz with stretching for 1Mhz as needed)
 *                                        |      |
 *                                        |      o<-- ~phi1  (phi1 is inverted phi0, so this *  is basically
 *                                        +------+            a slightly phase shifted phi.)
 *
 * Truth table:
 * +-----+-----+-----+---------------+-----------+
 * |  V  |  ~V | A15 | NAND(A15, ~V) | phi | 0=en|
 * +-----+-----+-----+---------------+-----+-----+
 * |  0  |  1  |  0  |       1       |  0  |  1  |
 * |  0  |  1  |  0  |       1       |  1  |  0  |
 * |  0  |  1  |  1  |       0       |  0  |  1  |
 * |  0  |  1  |  1  |       0       |  1  |  1  |
 * |  1  |  0  |  0* |       1       |  0  |  1  |
 * |  1  |  0  |  0* |       1       |  1  |  0  |
 * |  1  |  0  |  1  |       1       |  0  |  1  |
 * |  1  |  0  |  1  |       1       |  1  |  0  |
 * +-----+-----+-----+---------------+-----+-----+
 *
 * Enable when phi is high and the write address is ULA or DRAM
 */
bool Beeb::data_bus_isolated() {
  // Isolated when phi is low
  if (clock_->is_low(CLK_2_MHZ)) return true;  

  // Isolated unless weriting to vULA regs or DRAM
  auto addr = bus_->get_address();
  if ((addr > DRAM_LAST) &&
      addr != MMIO_VULA_REG_SEL &&
      addr == MMIO_VULA_PLT)
    return true;
  return false;
}
FUCKSICKLE
bool Beeb::address_bus_isolated() {
  return false;
}
SHITWITTERY


void Beeb::tick() {
  clock_->tick();

//      v_ula_->tick(bus_);

  if (clock_->went_low(CLK_2_MHZ)) {
    cpu_->tick(bus_);
  }

  if (clock_->went_low(CLK_4_MHZ)) {

    // TODO: Consider making DramBus a subclass of Bus and move this into it.
    if (!address_bus_isolated()) {
      dram_bus_->set_address(bus_->get_address());
      if ((bus_->tst_RW() == 1) & !data_bus_isolated()) {
        dram_bus_->set_data(bus_->get_data());
      }
    }

    dram_->tick(dram_bus_);

    if ((bus_->tst_RW() == 0) && !data_bus_isolated()) {
      bus_->set_data(dram_bus_->get_data());
    }
    mos_->tick(bus_);
  }

//      system_via_->tick(bus_);
//      user_via_->tick(bus_);
//      keyboard_->tick();
//      latch_->tick();
//      sound_chip_->tick();
//      acia_->tick(bus_);
//      adc_->tick(bus_);
}

std::vector<uint8_t> Beeb::get_memory_contents(uint16_t start_addr, uint32_t num_bytes) const {
  if (num_bytes == 0) {
    spdlog::warn("BEEB: Request for 0 bytes of data in get_memory_contents at {:04x}", start_addr);
    return {};
  }
  if (start_addr + num_bytes > 0x10000) {
    spdlog::error("BEEB: Retrieving {} bytes from start address {:04x} goes beyond 64K boudnary.", num_bytes,
                  start_addr);
    return {};
  }

  std::vector<uint8_t> return_data;
  if (start_addr <= DRAM_LAST) {
    const auto &dram_memory = dram_->data();
    // Copy at least some of the data from DRAM
    auto nb = std::min((uint32_t) num_bytes, (uint32_t) DRAM_LAST - (uint32_t) start_addr + 1);
    return_data.insert(return_data.end(),
                       dram_memory->begin() + start_addr,
                       dram_memory->begin() + start_addr + nb);
    num_bytes -= nb;
    start_addr += nb;
  }

  // For now, BASIC ROM
  if (num_bytes > 0 && start_addr <= BASIC_ROM_LAST) {
    const auto &basic_memory = basic_rom_->data();
    // Copy at least some of the data from DRAM
    auto nb = std::min((uint32_t) num_bytes, (uint32_t) BASIC_ROM_LAST - start_addr + 1);

    // Need to adjust the actual addresses to offset the base address in memory
    return_data.insert(return_data.end(),
                       basic_memory->begin() + start_addr - BASIC_ROM_BASE,
                       basic_memory->begin() + start_addr - BASIC_ROM_BASE + nb);
    num_bytes -= nb;
    start_addr += nb;
  }

  // MOS
  if (num_bytes > 0 /* Add MMIO HW exclusions here */) {
    const auto &mos_memory = mos_->data();
    // Copy at least some of the data from DRAM
    return_data.insert(return_data.end(),
                       mos_memory->begin() + start_addr - MOS_ROM_BASE,
                       mos_memory->begin() + start_addr - MOS_ROM_BASE + num_bytes);
  }

  return return_data;
}
