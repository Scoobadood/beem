#include "beeb.h"
#include "rom.h"
#include "clock.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <fstream>
#include <memory>

/* Memory Map constants */
const uint16_t DRAM_BASE = 0x0000;
const uint16_t DRAM_LAST = 0x7fff;
const uint16_t DRAM_SIZE = DRAM_LAST - DRAM_BASE + 1;

const uint16_t BASIC_ROM_BASE = 0x8000;
const uint16_t BASIC_ROM_LAST = 0xBfff;
const uint16_t MOS_ROM_BASE = 0xC000;

/* MMIO addresses */
const uint16_t MMIO_CRTC_REG_SEL = 0xfe00;
const uint16_t MMIO_CRTC_READ_WRITE = 0xfe01;
const uint16_t MMIO_ACIA_START = 0xfe08;
const uint16_t MMIO_ACIA_END = 0xfe0f;
const uint16_t MMIO_SULA_START = 0xfe10;
const uint16_t MMIO_SULA_END = 0xfe1f;
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
    : read_pages_{} //
    , write_pages_{} //
    , cached_dram_bus_{0} //
    , cached_irq_{false} //
    , last_cassette_motor_{false} //
{
  using namespace std;


  // Make a common clock for most things
  clock_ = make_shared<Clock>();

  cpu_ = make_shared<M6502>();

  bus_ = make_shared<Bus>();
  dram_bus_ = make_shared<Bus>();
  dram_ = make_shared<DRAM>(DRAM_SIZE, DRAM_BASE, clock_);
  basic_rom_ = make_shared<Rom>("data/Basic2.rom", BASIC_ROM_BASE);
  mos_ = make_shared<Rom>("data/os120.bin", MOS_ROM_BASE);

  system_via_ = std::make_unique<Via>(0xfe40);
  user_via_ = std::make_unique<Via>(0xfe60);

  latch_ = std::make_unique<IC32Latch>();
  system_via_->subscribe_port_b(latch_->src());

  // Attach Sound chip
  sound_chip_ = std::make_unique<SN76489>();
  latch_->subscribe(sound_chip_->we_src());
  system_via_->subscribe_port_a(sound_chip_->data_src());

  // Attach keyboard
  keyboard_ = std::make_unique<Keyboard>(0x07 - (boot_mode & 0x07));
  latch_->subscribe(keyboard_->we_src());
  latch_->subscribe(keyboard_->cl_led_src());
  latch_->subscribe(keyboard_->sl_led_src());
  system_via_->subscribe_port_a(keyboard_->data_src());
  system_via_->provide_port_a(keyboard_->provider());
  system_via_->provide_ca2(keyboard_->irq_provider());

  class dsp : public data_provider_8_bit {
   public:
    ~dsp() override = default;

    inline bool has_data() const override { return true; }

    inline uint8_t data() override { return 0x80; }
  };
  dummy_speech_provider_ = make_shared<dsp>();
  system_via_->provide_port_b(dummy_speech_provider_);

  // ACIA
  acia_ = make_shared<Acia>(0xfe08);

  // Serial ULA
  s_ula_ = make_shared<SerialUla>(0xfe10);
  s_ula_->set_acia(acia_);


  // ADC
  adc_ = std::make_unique<Adc>(0xfec0);


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

  init_page_table();

  // Register SHEILA devices in address order.
  // VideoUla is excluded — see IBusDevice comment in i_bus_device.h.
  sheila_devices_ = {
      crtc_.get(),
      acia_.get(),
      s_ula_.get(),
      system_via_.get(),
      user_via_.get(),
      adc_.get(),
  };

  // Only devices that can assert /IRQ. Checked every CLK_E_2_MHZ tick so
  // kept small. Add new IRQ-capable devices here when they are implemented.
  irq_devices_ = {
      system_via_.get(),
      user_via_.get(),
      acia_.get(),
  };
}

void Beeb::init_page_table() {
  // All pages default to null (MMIO / unmapped)
  memset(read_pages_,  0, sizeof(read_pages_));
  memset(write_pages_, 0, sizeof(write_pages_));

  // DRAM: pages 0x00–0x7F (32 KB)
  uint8_t* dram_base = dram_->raw_ptr();
  for (int p = 0x00; p <= 0x7F; ++p) {
    read_pages_[p]  = dram_base + p * 256;
    write_pages_[p] = dram_base + p * 256;
  }

  // BASIC ROM: pages 0x80–0xBF (read-only)
  uint8_t* basic_base = const_cast<uint8_t*>(basic_rom_->raw_ptr());
  for (int p = 0x80; p <= 0xBF; ++p) {
    read_pages_[p]  = basic_base + (p - 0x80) * 256;
    write_pages_[p] = nullptr;  // ROM: writes ignored
  }

  // MOS ROM: pages 0xC0–0xFB and 0xFF (read-only)
  uint8_t* mos_base = const_cast<uint8_t*>(mos_->raw_ptr());
  for (int p = 0xC0; p <= 0xFB; ++p) {
    read_pages_[p]  = mos_base + (p - 0xC0) * 256;
    write_pages_[p] = nullptr;
  }
  read_pages_[0xFF]  = mos_base + (0xFF - 0xC0) * 256;
  write_pages_[0xFF] = nullptr;

  // Pages 0xFC–0xFE: SHEILA / FRED / JIM — leave null on both tables
  // (null read_pages_ + null write_pages_ → MMIO path)
}

uint8_t Beeb::handle_mmio_read(uint16_t addr) {
  bus_->set_RW();
  bus_->set_address(addr);
  // Dispatch to the owning device by re-using their existing tick logic.
  // Each device checks its own address range and responds if matched.
  mos_->tick(bus_);
  basic_rom_->tick(bus_);
  return bus_->get_data();
}

void Beeb::handle_mmio_write(uint16_t addr, uint8_t data) {
  bus_->clr_RW();
  bus_->set_address(addr);
  bus_->set_data(data);
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
 * Return true if the device at addr runs on the 1MHz bus and requires
 * cycle-stretching. FRED (0xfc00) and JIM (0xfd00) are always 1MHz pages.
 * All other SHEILA devices report their own speed via IBusDevice.
 * Data source: https://beebwiki.mdfs.net/Cycle_stretching
 */
bool Beeb::is_1mhz_device_address(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();

  // Fast path: DRAM (0x0000-0x7fff) and most of ROM (0x8000-0xfbff) are never
  // 1MHz devices. This covers the vast majority of CPU accesses with one compare.
  if (addr < MMIO_FRED_START) return false;

  if (addr >= MMIO_FRED_START && addr <= MMIO_FRED_END) return true;
  if (addr >= MMIO_JIM_START  && addr <= MMIO_JIM_END)  return true;

  for (auto* dev : sheila_devices_) {
    if (dev->decodes(addr)) return dev->is_1mhz_device();
  }
  return false;
}


void Beeb::tick() {
  clock_->tick();

  if (clock_->went_high(CLK_2_MHZ)) {
    cached_dram_bus_ = dram_bus_->get_pins();
  }

  // CPU normally does internal work in LOW phase and then
  // Bus RW in high phase. We're phaking it so we just go
  // Off the high phase which also makes the isolation code work.
  if (clock_->went_high(CLK_E_2_MHZ)) {
    if (cached_irq_) cpu_->raise_irq(); else cpu_->clear_irq();

    cpu_->tick(bus_);

    if (is_1mhz_device_address(bus_)) {
      clock_->begin_time_stretch();
    }

  }
  // Memory access
  if (clock_->went_high(CLK_4_MHZ)) {
    if (cpu_has_address_bus()) {
      // CPU phase: O(1) page-table dispatch
      uint16_t addr = bus_->get_address();
      uint8_t page = addr >> 8;

      if (bus_->tst_RW()) {
        // Read
        if (read_pages_[page]) {
          bus_->set_data(read_pages_[page][addr & 0xFF]);
        }
        // else: MMIO read — devices tick themselves at 1MHz (CLK_1_MHZ handler)
      } else {
        // Write
        if (write_pages_[page]) {
          write_pages_[page][addr & 0xFF] = bus_->get_data();
        }
        // else: ROM write (ignored) or MMIO write (handled at 1MHz)
      }
    } else {
      // CRTC phase: still uses dram_bus_ — unchanged until Phase D
      dram_->tick(dram_bus_);
    }
  }

  if (clock_->went_low(CLK_2_MHZ)) {
    dram_bus_->set_pins(cached_dram_bus_);
  }

/* Tick the 1MHz stuff */
  if (clock_->went_low(CLK_1_MHZ)) {
    keyboard_->tick();

    // VIA timers must tick every 1MHz cycle regardless of CPU address.
    // MMIO is handled via the SHEILA registry dispatch below.
    system_via_->tick_timers();
    user_via_->tick_timers();

    latch_->tick();

    auto addr = bus_->get_address();
    if ((addr & 0xff00) == 0xfe00) {
      // Watch for VIA Timer 1 and IER writes specifically
      if (!bus_->tst_RW() && (addr == 0xfe45 || addr == 0xfe4e)) {
        spdlog::info("SHEILA VIA key write: addr={:04x} data={:02x}", addr, bus_->get_data());
      }
      for (auto* dev : sheila_devices_) {
        if (dev->decodes(addr)) { dev->tick(bus_); break; }
      }
    }

//    sound_chip_->tick();

    // Update IRQ cache after all 1MHz device ticks — IRQ state can only change here.
    cached_irq_ = false;
    for (auto* dev : irq_devices_) cached_irq_ |= dev->has_irq();

    // Step 6 diagnostic: log first IRQ delivery and which device raised it.
    static bool first_irq_logged = false;
    if (!first_irq_logged && cached_irq_) {
      first_irq_logged = true;
      spdlog::info("BEEB: First IRQ — sysvia={} uservia={} acia={}",
                   irq_devices_[0]->has_irq(),
                   irq_devices_[1]->has_irq(),
                   irq_devices_[2]->has_irq());
    }

    // notify listeners of stuff
    auto scm = s_ula_->is_motor_on();
    if (scm != last_cassette_motor_) {
      for (const auto &fn : cassette_listeners_) {
        fn(scm);
      }
      last_cassette_motor_ = scm;
    }
  }

  if (clock_->went_low(CLK_16_MHZ)) {
    v_ula_->tick(bus_, dram_bus_);
    crt_->tick();

    s_ula_->tick_16mhz();
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

void Beeb::get_memory_contents(uint16_t start_addr, uint32_t num_bytes, uint8_t * buffer) const {
  if (num_bytes == 0) {
    spdlog::warn("BEEB: Request for 0 bytes of data in get_memory_contents at {:04x}", start_addr);
    return;
  }
  if (start_addr + num_bytes > 0x10000) {
    spdlog::error("BEEB: Retrieving {} bytes from start address {:04x} goes beyond 64K boudnary.", num_bytes,
                  start_addr);
    return;
  }

  auto buffer_idx = 0;
  if (start_addr <= DRAM_LAST) {
    const auto &dram_memory = dram_->data();
    // Copy at least some of the data from DRAM
    auto nb = std::min((uint32_t) num_bytes, (uint32_t) DRAM_LAST - (uint32_t) start_addr + 1);
    for( auto i=0; i<nb; ++i) {
      buffer[buffer_idx++] = dram_memory->at(start_addr+i);
    }
    num_bytes -= nb;
    start_addr += nb;
  }

  // For now, BASIC ROM
  if (num_bytes > 0 && start_addr <= BASIC_ROM_LAST) {
    const auto &basic_memory = basic_rom_->data();
    // Copy at least some of the data from DRAM
    auto nb = std::min((uint32_t) num_bytes, (uint32_t) BASIC_ROM_LAST - start_addr + 1);

    // Need to adjust the actual addresses to offset the base address in memory
    for( auto i=0; i<nb; ++i) {
      buffer[buffer_idx++] = basic_memory->at(start_addr+i-BASIC_ROM_BASE);
    }
    num_bytes -= nb;
    start_addr += nb;
  }

  // MOS
  if (num_bytes > 0 /* Add MMIO HW exclusions here */) {
    const auto &mos_memory = mos_->data();
    // Copy at least some of the data from DRAM
    for( auto i=0; i<num_bytes; ++i) {
      buffer[buffer_idx++] = mos_memory->at(start_addr+i-MOS_ROM_BASE);
    }
  }
}

void Beeb::press_key(uint8_t key_code) {
  keyboard_->press_key(key_code);
}

void Beeb::release_key(uint8_t key_code) {
  keyboard_->release_key(key_code);
}

void Beeb::load_data(const std::vector<uint8_t> &data, uint16_t address) {
  dram_->load(data, address);
}

void Beeb::add_cassette_listener(const std::function<void(bool)> &listener) {
  cassette_listeners_.push_back(listener);
}

void Beeb::set_cassette_port(ICassettePort* port) {
  s_ula_->set_cassette_port(port);
  // Sync current motor state so the port doesn't start cold.
  if (port)
    port->set_motor(s_ula_->is_motor_on());
}