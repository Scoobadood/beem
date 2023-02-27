//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef M6502_SRC_BEEB_H_
#define M6502_SRC_BEEB_H_

#include "dram.h"
#include "m6502.h"
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

using VideoHandler = std::function<void(int32_t w, int32_t h, std::vector<uint8_t> scr_data)>;

const uint32_t SCR_WIDTH = 128 * 8; // 1024
const uint32_t SCR_HEIGHT = 39 * 8; // 312

class Beeb {
public:
  explicit Beeb(uint8_t boot_mode = 7);

  void tick();

  void reset();

  std::shared_ptr<M6502> cpu() { return cpu_; }

  std::shared_ptr<DRAM> memory() { return dram_; }

  std::shared_ptr<Rom> mos() { return mos_; }

  std::shared_ptr<Bus> bus() { return bus_; }

  void press_key(uint8_t key_code);

  void release_key(uint8_t key_code);

  void set_video_handler(const VideoHandler &fn) { fn_ = fn; }

  [[nodiscard]] std::vector<uint8_t> get_memory_contents(uint16_t start_addr, uint32_t num_bytes) const;

private:
  bool cpu_has_address_bus();

  void pre_dram_checks();

  void post_dram_checks();

  void update_screen();

  static bool is_1mhz_device_address(const std::shared_ptr<Bus> &bus);

  std::shared_ptr<Clock> clock_;
  std::shared_ptr<M6502> cpu_;
  std::shared_ptr<DRAM> dram_;
  std::shared_ptr<Bus> bus_;
  std::shared_ptr<Bus> dram_bus_;
  std::shared_ptr<Rom> basic_rom_;
  std::shared_ptr<Rom> mos_;
  std::shared_ptr<data_provider_8_bit> dummy_speech_provider_;
  std::shared_ptr<VideoUla> v_ula_;
  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<Crt> crt_;

  Via *system_via_;
  Via *user_via_;
  IC32Latch *latch_;
  SN76489 *sound_chip_;
  Keyboard *keyboard_;
  Acia *acia_;
  Adc *adc_;

  // Call back for monitor
  VideoHandler fn_;

  uint64_t cached_dram_bus_;
};

#endif //M6502_SRC_BEEB_H_
