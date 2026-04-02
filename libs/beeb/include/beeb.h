//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_SRC_BEEB_H_
#define M6502_SRC_BEEB_H_

#include "dram.h"
#include <6502/m6502.h>
#include "6522_via.h"
#include "IC32Latch.h"
#include "sound_76489.h"
#include "keyboard.h"
#include "acia_6850.h"
#include "adc.h"
#include "rom.h"
#include "5c094_vula.h"
#include "6845_crtc.h"
#include "crt.h"
#include "clock.h"
#include "../../hardware/2C198_sULA/include/2c198_sula.h"
#include "../../hardware/2C198_sULA/include/i_cassette_port.h"
#include "i_bus_device.h"
#include <vector>

class Beeb {
public:
  explicit Beeb(uint8_t boot_mode = 7);

  void tick();

  void reset();

  std::shared_ptr<M6502> cpu() { return cpu_; }

  std::shared_ptr<DRAM> memory() { return dram_; }

  std::shared_ptr<Rom> mos() { return mos_; }

  std::shared_ptr<Bus> bus() { return bus_; }

  std::shared_ptr<Crt> crt() { return crt_; }

  void press_key(uint8_t key_code);

  void release_key(uint8_t key_code);

  [[nodiscard]] std::vector<uint8_t> get_memory_contents(uint16_t start_addr, uint32_t num_bytes) const;
  void get_memory_contents(uint16_t start_addr, uint32_t num_bytes, uint8_t * buffer) const;

  void load_data(const std::vector<uint8_t>& data, uint16_t address);

  void add_cassette_listener(const std::function<void(bool)>& listener);

  // Plug a cassette port device into the machine's serial port connector.
  // Ownership stays with the caller. Pass nullptr to disconnect.
  void set_cassette_port(ICassettePort* port);

 private:
  bool cpu_has_address_bus();

  void init_page_table();

  uint8_t handle_mmio_read(uint16_t addr);

  void handle_mmio_write(uint16_t addr, uint8_t data);

  bool is_1mhz_device_address(const std::shared_ptr<Bus> &bus);

  std::shared_ptr<Clock> clock_;
  std::shared_ptr<M6502> cpu_;
  std::shared_ptr<DRAM> dram_;
  std::shared_ptr<Bus> bus_;
  std::shared_ptr<Bus> dram_bus_;
  std::shared_ptr<Rom> basic_rom_;
  std::shared_ptr<Rom> mos_;
  std::shared_ptr<data_provider_8_bit> dummy_speech_provider_;
  std::shared_ptr<VideoUla> v_ula_;
  std::shared_ptr<SerialUla> s_ula_;
  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<Crt> crt_;

  std::unique_ptr<Via> system_via_;
  std::unique_ptr<Via> user_via_;
  std::unique_ptr<IC32Latch> latch_;
  std::unique_ptr<SN76489> sound_chip_;
  std::unique_ptr<Keyboard> keyboard_;
  std::shared_ptr<Acia> acia_;
  std::unique_ptr<Adc> adc_;

  // Page dispatch table — indexed by addr >> 8.
  // Non-null: pointer to first byte of that 256-byte page (DRAM or ROM).
  // Null on read_pages_: MMIO or unmapped.
  // Null on write_pages_: ROM (ignore write) or MMIO (if read_pages_ also null).
  uint8_t*       read_pages_[256];
  uint8_t*       write_pages_[256];

  // SHEILA device registry — used for MMIO dispatch and cycle-stretch detection.
  // Does not own the devices; existing members do.
  std::vector<IBusDevice*> sheila_devices_;

  // Subset of sheila_devices_ that can raise /IRQ. Kept separate so the IRQ
  // aggregation loop (runs every CLK_E_2_MHZ tick) stays O(k) where k is small.
  std::vector<IBusDevice*> irq_devices_;

  uint64_t cached_dram_bus_;
  bool cached_irq_;

  std::vector<std::function<void(bool)>> cassette_listeners_;
  bool last_cassette_motor_;
};

#endif //M6502_SRC_BEEB_H_
