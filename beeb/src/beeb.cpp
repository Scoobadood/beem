#include "beeb.h"
#include "rom.h"
#include "clock.h"

#include  <fstream>
#include  <iostream>

#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>

/* Memory Map constants */
const uint16_t DRAM_BASE = 0x0000;
const uint16_t DRAM_LAST = 0x7fff;
const uint16_t DRAM_SIZE = DRAM_LAST - DRAM_BASE + 1;

const uint16_t BASIC_ROM_BASE = 0x8000;
const uint16_t BASIC_ROM_LAST = 0xBfff;
const uint16_t BASIC_ROM_SIZE = BASIC_ROM_LAST - BASIC_ROM_BASE + 1;
const uint16_t MOS_ROM_BASE = 0xC000;

/* MMIO addresses */
const uint16_t MMIO_SULA_START = 0xfe08;
const uint16_t MMIO_SULA_END = 0xfe1f;
const uint16_t MMIO_CRTC_REG_SEL = 0xfe00;
const uint16_t MMIO_CRTC_READ_WRITE = 0xfe01;
const uint16_t MMIO_ACIA = 0xfe08;
const uint16_t MMIO_ECONET_STATID = 0xfe18;
const uint16_t MMIO_VULA_REG_SEL = 0xfe20;
const uint16_t MMIO_VULA_PLT = 0xfe21;
const uint16_t MMIO_SYSTEM_VIA_START = 0xfe40;
const uint16_t MMIO_SYSTEM_VIA_END = 0xfe4f;
const uint16_t MMIO_USER_VIA_START = 0xfe60;
const uint16_t MMIO_USER_VIA_END = 0xfe6f;
const uint16_t MMIO_ADC_START = 0xfec0;
const uint16_t MMIO_ADC_END = 0xfec2;
const uint16_t MMIO_FRED_START = 0xfc00;
const uint16_t MMIO_FRED_END = 0xfcff;
const uint16_t MMIO_JIM_START = 0xfd00;
const uint16_t MMIO_JIM_END = 0xfdff;

Beeb::Beeb() {
  using namespace std;

  try {
    auto logger = spdlog::basic_logger_mt("BusDance", "logs/bus-dance-log.txt", true);
    logger->flush_on(spdlog::level::debug);

    logger = spdlog::basic_logger_mt("DebugCRTC", "logs/beeb-crtc-log.txt", true);
    logger->flush_on(spdlog::level::debug);

  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }


  // Make a common clock for most things
  clock_ = make_shared<Clock>();

  cpu_ = make_shared<M6502>();

  bus_ = make_shared<Bus>();
  dram_bus_ = make_shared<Bus>();
  dram_ = make_shared<DRAM>(DRAM_SIZE, DRAM_BASE, clock_);
  basic_rom_ = make_shared<Rom>("data/Basic2.rom", BASIC_ROM_BASE);
  mos_ = make_shared<Rom>("data/os120.bin", MOS_ROM_BASE);

  system_via_ = new Via(0xfe40);
  user_via_ = new Via(0xfe60);

  latch_ = new IC32Latch();
  system_via_->subscribe_port_b(latch_->src());

  // Attach Sound chip
  sound_chip_ = new SN76489();
  latch_->subscribe(sound_chip_->we_src());
  system_via_->subscribe_port_a(sound_chip_->data_src());

  // Attach keyboard
  keyboard_ = new Keyboard(0x01 /* Boot into mode 6 */);
  latch_->subscribe(keyboard_->we_src());
  latch_->subscribe(keyboard_->cl_led_src());
  latch_->subscribe(keyboard_->sl_led_src());
  system_via_->subscribe_port_a(keyboard_->data_src());
  system_via_->provide_port_a(keyboard_->provider());

  // ACIA
  acia_ = new Acia();

  // ADC
  adc_ = new Adc(0xfec0);


  // Video ULA
  v_ula_ = new VideoUla(0xfe20);
  v_ula_->set_clock(clock_);

  // CRTC
  crtc_ = new Crtc(0xfe00, clock_);
  latch_->subscribe(crtc_->hw_scroll_addr());
  v_ula_->set_crtc(crtc_);

  screen_Data_ = new uint8_t[640 * 256 * 3];
  pixel_x_ = pixel_y_ = 0;
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
  if (clock_->is_low(CLK_E_2_MHZ)) return true;

  // Isolated unless writing to vULA regs or DRAM
  auto addr = bus_->get_address();
  if ((addr > DRAM_LAST) &&
      addr != MMIO_VULA_REG_SEL &&
      addr == MMIO_VULA_PLT)
    return true;
  return false;
}

bool Beeb::cpu_has_address_bus() {
  return clock_->is_high(CLK_E_2_MHZ);
}

/*
 * Return true if the address on the bus corresponds to 1MHz HW, specifically:
 * -
 *
 */
bool Beeb::is_1mhz_device_address(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();

  /* CRTC */
  if (addr == MMIO_CRTC_READ_WRITE || addr == MMIO_CRTC_REG_SEL) return true;

  /* ACIA */
  if (addr == MMIO_ACIA) return true;

  /* Serial ULA */
  if (addr >= MMIO_SULA_START && addr <= MMIO_SULA_END) return true;

  /* Econet station id */
  if (addr >= MMIO_ECONET_STATID) return true;

  /* Two VIAs */
  if (addr >= MMIO_SYSTEM_VIA_START && addr <= MMIO_SYSTEM_VIA_END) return true;
  if (addr >= MMIO_USER_VIA_START && addr <= MMIO_USER_VIA_END) return true;

  if (addr >= MMIO_ADC_START && addr <= MMIO_ADC_END) return true;

  if (addr >= MMIO_FRED_START && addr <= MMIO_FRED_END) return true;
  if (addr >= MMIO_JIM_START && addr <= MMIO_JIM_END) return true;

  return false;
}

/* Determine whether we should copy address and data from main bus or not*/
void Beeb::pre_dram_checks() {
  if (cpu_has_address_bus()) {

    spdlog::get("BusDance")->debug("Beeb PreD: CPU has address bus");
    spdlog::get("BusDance")->debug("         : 16:{}, 8:{}, 4:{}, 2E:{}, 2:{}, 1:{}",
                                   clock_->is_high(CLK_16_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_8_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_4_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_E_2_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_2_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_1_MHZ) ? "H" : "L"
    );
    auto addr = bus_->get_address();
    if (addr >= DRAM_BASE && addr <= DRAM_LAST) {
      spdlog::get("BusDance")->debug("        : Addressing DRAM");
      spdlog::get("BusDance")->debug("         : Setting addr: {:04x}", addr);
      spdlog::get("BusDance")->debug("         : RW is {}", bus_->tst_RW() ? "R" : "W");
      dram_bus_->set_address(addr);
      if (bus_->tst_RW()) {
        spdlog::get("BusDance")->debug("         : Attempting read");
        dram_bus_->set_RW();
      } else {
        dram_bus_->clr_RW();
        spdlog::get("BusDance")->debug("         : Attempting write");
        if (data_bus_isolated()) {
          spdlog::get("BusDance")->debug("         : Data bus is isolated. No further action.");
        } else {
          spdlog::get("BusDance")->debug("         : Setting DRAM bus to {:02x}", bus_->get_data());
          dram_bus_->set_data(bus_->get_data());
        }
      } // else (writing)
    } else {
      spdlog::get("BusDance")->debug("         : Address {:04x} not in DRAM", addr);
    } // Adress not in DRAM
  } else {
    spdlog::get("BusDance")->debug("Beeb PreD:CPU does not have address bus");
    spdlog::get("BusDance")->debug("         : 16:{}, 8:{}, 4:{}, 2E:{}, 2:{}, 1:{}",
                                   clock_->
                                           is_high(CLK_16_MHZ)
                                   ? "H" : "L",
                                   clock_->
                                           is_high(CLK_8_MHZ)
                                   ? "H" : "L",
                                   clock_->
                                           is_high(CLK_4_MHZ)
                                   ? "H" : "L",
                                   clock_->
                                           is_high(CLK_E_2_MHZ)
                                   ? "H" : "L",
                                   clock_->
                                           is_high(CLK_2_MHZ)
                                   ? "H" : "L",
                                   clock_->
                                           is_high(CLK_1_MHZ)
                                   ? "H" : "L"
    );
  }
}

void Beeb::post_dram_checks() {
  if (cpu_has_address_bus()) {
    spdlog::get("BusDance")->debug("Beeb PstD: CPU has address bus");
    spdlog::get("BusDance")->debug("         : 16:{}, 8:{}, 4:{}, 2E:{}, 2:{}, 1:{}",
                                   clock_->is_high(CLK_16_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_8_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_4_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_E_2_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_2_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_1_MHZ) ? "H" : "L"
    );
    auto addr = bus_->get_address();
    if (addr >= DRAM_BASE && addr <= DRAM_LAST) {
      spdlog::get("BusDance")->debug("        : Addressing DRAM");
      spdlog::get("BusDance")->debug("         : RW is {}", bus_->tst_RW() ? "R" : "W");
      if (bus_->tst_RW()) {
        spdlog::get("BusDance")->debug("         : Attempting read");
        if (data_bus_isolated()) {
          spdlog::get("BusDance")->debug("         : Data bus isolated.");
          bus_->set_data(0);
        } else {
          spdlog::get("BusDance")->debug("         : Read {:02x} from DRAM", dram_bus_->get_data());
          bus_->set_data(dram_bus_->get_data());
        }
      } else {
        spdlog::get("BusDance")->debug("         : Write happened");
      }
    } else {
      spdlog::get("BusDance")->debug("         : Address {:04x} not in DRAM", addr);
    }
  } else {
    spdlog::get("BusDance")->debug("Beeb PstD:CPU does not have address bus");
    spdlog::get("BusDance")->debug("         : 16:{}, 8:{}, 4:{}, 2E:{}, 2:{}, 1:{}",
                                   clock_->is_high(CLK_16_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_8_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_4_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_E_2_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_2_MHZ) ? "H" : "L",
                                   clock_->is_high(CLK_1_MHZ) ? "H" : "L"
    );
  }
}

void Beeb::tick() {
  static int scrid = 0;

  clock_->tick();

// CPU normally does internal work in LOW phase and then
// Bus RW in high phase. We're phaking it so we just go
// Off the high phase which also makes the isolation code work.
  if (clock_->went_high(CLK_E_2_MHZ)) {
    cached_dram_bus_ = dram_bus_->get_pins();
    spdlog::get("DebugCRTC")->debug("CPU   : 2MHZE went high. Caching DRAM bus A:{:04x} RW:{} D:{:02x}",
                                    dram_bus_->get_address(),
                                    dram_bus_->tst_RW() ? "R" : "W",
                                    bus_->get_data());
    spdlog::get("DebugCRTC")->debug("      : Main Bus A:{:04x} RW:{} D:{:02x}",
                                    bus_->get_address(),
                                    bus_->tst_RW() ? "R" : "W",
                                    bus_->get_data());

    spdlog::get("BusDance")->debug("Beeb         : Caching current DRAM data {:02x}", dram_bus_->get_data());

    cpu_->tick(bus_);

    // TODO DEBUGGIN VULA remove me
    spdlog::get("DebugCRTC")->debug("CPU   : Finished cycle with A:{:04x}, RnW:{}, {}",
                                    bus_->get_address(),
                                    bus_->tst_RW() ? "R" : "W",
                                    bus_->tst_RW() ? "" : fmt::format("D:{:02x}", bus_->get_data()));
    spdlog::get("DebugCRTC")->debug("      : DRAM Bus : A:{:04x}, RnW:{}, {}",
                                    dram_bus_->get_address(),
                                    dram_bus_->tst_RW() ? "R" : "W",
                                    dram_bus_->tst_RW() ? "" : fmt::format("D:{:02x}", dram_bus_->get_data()));
    // TODO End

    if (is_1mhz_device_address(bus_)) {
      clock_->begin_time_stretch();
    }
  }

  if (clock_->went_low(CLK_E_2_MHZ)) {
    spdlog::get("DebugCRTC")->debug("CPU   : 2MHZE went low. DRAM bus before restore A:{:04x} RW:{} D:{:02x}",
                                    dram_bus_->get_address(),
                                    dram_bus_->tst_RW() ? "R" : "W",
                                    dram_bus_->get_data());
    spdlog::get("DebugCRTC")->debug("      : Main bus A:{:04x}, RnW:{}, {:02x}",
                                    bus_->get_address(),
                                    bus_->tst_RW() ? "R" : "W",
                                    bus_->get_data());

    dram_bus_->set_pins(cached_dram_bus_);
    spdlog::get("DebugCRTC")->debug("      : DRAM bus after restore A:{:04x} RW:{} D:{:02x}",
                                    dram_bus_->get_address(),
                                    dram_bus_->tst_RW() ? "R" : "W",
                                    dram_bus_->get_data());
    spdlog::get("BusDance")->debug("Beeb         : Restoring DRAM data from cache {:02x}", dram_bus_->get_data());
  }

/* Tick the 1MHz stuff */
  if (clock_->went_low(CLK_1_MHZ)) {
    system_via_->tick(bus_);
    user_via_->tick(bus_);

    keyboard_->tick();
    latch_->tick();

    /*
     * Always poke CRTC to read bus at 1MHz because
     *
     * "Enable (E) - The enable signal is a high-impedance TTL/MOS
     * compatible input which enables the data bus input/output
     * buffers and clocks data to and from the CRTC. This Signal
     * is usually derived from the processor clock. The high-to-low
     * transition is the active edge."
     */
    crtc_->tick(bus_);


//    sound_chip_->tick();
//    acia_->tick(bus_);
//    adc_->tick(bus_);
  }

  if (clock_->went_low(CLK_4_MHZ)) {
    pre_dram_checks();
    dram_->tick(dram_bus_);
    post_dram_checks();
    mos_->tick(bus_);
  }

  if (clock_->went_low(CLK_16_MHZ)) {
    v_ula_->tick(bus_, dram_bus_);

    if (crtc_->display_enable()) {
      auto rgb = v_ula_->rgb();

      if( rgb != 0 ) {
        auto px = (pixel_addr_/3) % 640;
        auto py = (pixel_addr_/3) / 640;
        spdlog::info( "Set pixel at {},{}", px, py);
      }
      screen_Data_[pixel_addr_++] = (rgb >> 16) & 0xff;
      screen_Data_[pixel_addr_++] = (rgb >> 8) & 0xff;
      screen_Data_[pixel_addr_++] = rgb & 0xff;
      if (pixel_addr_ == 640 * 256 * 3) {
        pixel_addr_ = 0;
        auto scr_file_name = fmt::format("/Users/dave/Desktop/screen_{:02}.data", scrid);
        std::ofstream d{scr_file_name, std::ios::binary};
        d.write( (const char *)screen_Data_, 640 * 256 * 3);
        d.close();
        memset(screen_Data_, 0, 640 * 256 * 3);
        scrid = (scrid + 1) % 100;
      }
    }
  }
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
