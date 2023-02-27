#include "beeb.h"
#include "rom.h"
#include "clock.h"

#include <fstream>
#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>

/* Memory Map constants */
const uint16_t DRAM_BASE = 0x0000;
const uint16_t DRAM_LAST = 0x7fff;
const uint16_t DRAM_SIZE = DRAM_LAST - DRAM_BASE + 1;

const uint16_t BASIC_ROM_BASE = 0x8000;
const uint16_t BASIC_ROM_LAST = 0xBfff;
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

Beeb::Beeb(uint8_t boot_mode) //
        : cached_dram_bus_{0} //
{
  using namespace std;

  try {
    auto logger = spdlog::basic_logger_mt("CRT", "logs/crt-log.txt", true);
    logger->flush_on(spdlog::level::debug);
    logger = spdlog::basic_logger_mt("BusDance", "logs/bus-log.txt", true);
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
  keyboard_ = new Keyboard(0x07 - (boot_mode & 0x07));
  latch_->subscribe(keyboard_->we_src());
  latch_->subscribe(keyboard_->cl_led_src());
  latch_->subscribe(keyboard_->sl_led_src());
  system_via_->subscribe_port_a(keyboard_->data_src());
  system_via_->provide_port_a(keyboard_->provider());
  system_via_->provide_ca2(keyboard_->irq_provider());

  class dsp : public data_provider_8_bit {
  public:
    virtual ~dsp() = default;

    inline bool has_data() const override { return true; }

    inline uint8_t data() override { return 0x80; }
  };
  dummy_speech_provider_ = make_shared<dsp>();
  system_via_->provide_port_b(dummy_speech_provider_);

  // ACIA
  acia_ = new Acia();

  // ADC
  adc_ = new Adc(0xfec0);


  // Video ULA
  v_ula_ = make_shared<VideoUla>(0xfe20);
  v_ula_->set_clock(clock_);

  // CRTC
  crtc_ = make_shared<Crtc>(0xfe00);
  latch_->subscribe(crtc_->hw_scroll_addr());
  v_ula_->set_crtc(crtc_);
  system_via_->provide_ca1(crtc_->irq_provider());

  // CRT
  crt_ = make_shared<Crt>(crtc_, v_ula_);

  // Screen Data
  screen_data_.reserve(1280 * 768 * 3);
  pixel_x_ = 0;
  pixel_y_ = 0;
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
 * Service manual:
 * "Every 250 nanoseconds, control of the RAM address lines is switched between the microprocessor and the CRTC.
 * Thus, in each one microsecond period, the microprocessor has two RAM accesses and the CRTC has two RAM accesses."
 */
bool Beeb::cpu_has_address_bus() {
  return clock_->is_high(CLK_2_MHZ);
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
  if (!cpu_has_address_bus()) {
    spdlog::get("BusDance")->debug("PRE : CPU doesn't have control.");
    return;
  }

  auto addr = bus_->get_address();
  if (addr < DRAM_BASE || addr > DRAM_LAST) {
    spdlog::get("BusDance")->debug("PRE : CPU not accessing DRAM.");
    return;
  }

  dram_bus_->set_address(addr);
  if (bus_->tst_RW()) {
    dram_bus_->set_RW();
  } else {
    dram_bus_->clr_RW();
    dram_bus_->set_data(bus_->get_data());
  }
  spdlog::get("BusDance")->debug("PRE : CPU addressing DRAM. Copied Bus to DRAM bus {:04x} {:02x} {} {}",
                                 dram_bus_->get_address(),
                                 dram_bus_->get_data(),
                                 dram_bus_->tst_RW() ? "R" : "W",
                                 dram_bus_->tst_SYNC() ? "SYN" : "   ",
                                 dram_bus_->tst_RST() ? "RST" : "");
}

void Beeb::post_dram_checks() {
  if (!cpu_has_address_bus()) {
    spdlog::get("BusDance")->debug("POST: CPU doesn't have control.");
    return;
  }

  auto addr = bus_->get_address();
  if (addr < DRAM_BASE || addr > DRAM_LAST) {
    spdlog::get("BusDance")->debug("POST: CPU didn't access DRAM.");
    return;
  }

  if (bus_->tst_RW()) {
    bus_->set_data(dram_bus_->get_data());
  }
  spdlog::get("BusDance")->debug("POST: CPU had DRAM control and {} {:02x} {} {:04x}. {}",
                                 bus_->tst_RW() ? "read" : "wrote",
                                 bus_->get_data(),
                                 bus_->tst_RW() ? "from" : "to",
                                 bus_->get_address(),
                                 bus_->tst_RW() ? "Main bus updated" : ""
  );
}

void Beeb::tick() {
  clock_->tick();

  if (clock_->went_high(CLK_2_MHZ)) {
    spdlog::get("BusDance")->debug("BEEB: 2MHz went high. Cached DRAM bus {:04x} {:02x} {} {}",
                                   dram_bus_->get_address(),
                                   dram_bus_->get_data(),
                                   dram_bus_->tst_RW() ? "R" : "W",
                                   dram_bus_->tst_SYNC() ? "SYN" : "   ",
                                   dram_bus_->tst_RST() ? "RST" : "");
    cached_dram_bus_ = dram_bus_->get_pins();
  }

// CPU normally does internal work in LOW phase and then
// Bus RW in high phase. We're phaking it so we just go
// Off the high phase which also makes the isolation code work.
  if (clock_->went_high(CLK_E_2_MHZ)) {
    spdlog::get("BusDance")->debug("BEEB: 2MHzE went high. CPU starting work.");

    if (system_via_->has_irq()) cpu_->raise_irq(); else cpu_->clear_irq();

    cpu_->tick(bus_);
    spdlog::get("BusDance")->debug("CPU  : finished work. Main bus {:04x} {:02x} {} {}",
                                   bus_->get_address(),
                                   bus_->get_data(),
                                   bus_->tst_RW() ? "R" : "W",
                                   bus_->tst_SYNC() ? "SYN" : "   ",
                                   bus_->tst_RST() ? "RST" : "");

    if (is_1mhz_device_address(bus_)) {
      clock_->begin_time_stretch();
    }

  }
  // Allow for DRAM Access
  if (clock_->went_high(CLK_4_MHZ)) {
    spdlog::get("BusDance")->debug("BEEB: 4MHz went high. {} should have control",
                                   clock_->is_high(CLK_2_MHZ) ? " CPU" : "CRTC");
    pre_dram_checks();
    dram_->tick(dram_bus_);
    post_dram_checks();

    // Don't tick MOS for MMIO devices.
    if (bus_->get_address() <= 0xfc00 || bus_->get_address() >= 0xff00)
      mos_->tick(bus_);

    basic_rom_->tick(bus_);
  }

  if (clock_->went_low(CLK_2_MHZ)) {
    dram_bus_->set_pins(cached_dram_bus_);
    spdlog::get("BusDance")->debug("BEEB: 2MHz went low. DRAM bus restored {:04x} {:02x} {} {}",
                                   dram_bus_->get_address(),
                                   dram_bus_->get_data(),
                                   dram_bus_->tst_RW() ? "R" : "W",
                                   dram_bus_->tst_SYNC() ? "SYN" : "   ",
                                   dram_bus_->tst_RST() ? "RST" : "");
  }

/* Tick the 1MHz stuff */
  if (clock_->went_low(CLK_1_MHZ)) {
    keyboard_->tick();

    system_via_->tick(bus_);
    user_via_->tick(bus_);

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
    acia_->tick(bus_);
//    adc_->tick(bus_);
  }


  if (clock_->went_low(CLK_16_MHZ)) {
    v_ula_->tick(bus_, dram_bus_);
    update_screen();
  }
}

void Beeb::update_screen() {
  static bool last_vs;
  static bool last_hs;

  auto vs = crtc_->vsync();
  auto hs = crtc_->hsync();

  // HS just happened. Inc pix y and reset pix x
  if (!hs && last_hs) {
//    pixel_x_ = 0;
//    ++pixel_y_;
  }

  // VS just happened.
  // Send the screen and reset pix x and pix y
  if (!vs && last_vs) {
    fn_(1024, 312, screen_data_);
    pixel_x_ = 0;
    pixel_y_ = 0;
    screen_data_.clear();
  }

  if (++pixel_x_ == 1024) {
    pixel_x_ = 0;
    if (++pixel_y_ == 312) pixel_y_ = 0;
  }


  uint32_t pixel_colour = 0;
  if (crtc_->display_enable()) {
    pixel_colour = v_ula_->rgb();
  }
  screen_data_.push_back((pixel_colour >> 16) & 0xff);
  screen_data_.push_back((pixel_colour >> 8) & 0xff);
  screen_data_.push_back((pixel_colour >> 0) & 0xff);

  last_vs = vs;
  last_hs = hs;
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

void Beeb::press_key(uint8_t key_code) {
  keyboard_->press_key(key_code);
}

void Beeb::release_key(uint8_t key_code) {
  keyboard_->release_key(key_code);
}
