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
#include "clock.h"

class Beeb {
public:
  Beeb();

  void tick();

  void reset();

  std::shared_ptr<M6502> cpu() { return cpu_; }

  std::shared_ptr<DRAM> memory() { return dram_; }

  std::shared_ptr<Rom> mos() { return mos_; }

  std::shared_ptr<Bus> bus() { return bus_; }

  std::vector<uint8_t> get_memory_contents(uint16_t start_addr, uint32_t num_bytes) const;

private:
  bool data_bus_isolated();

  bool address_bus_isolated();

  std::shared_ptr<Clock> clock_;
  std::shared_ptr<M6502> cpu_;
  std::shared_ptr<DRAM> dram_;
  std::shared_ptr<Bus> bus_;
  std::shared_ptr<Bus> dram_bus_;
  std::shared_ptr<Rom> basic_rom_;
  std::shared_ptr<Rom> mos_;

  Via *system_via_;
  Via *user_via_;
  IC32Latch *latch_;
  SN76489 *sound_chip_;
  Keyboard *keyboard_;
  Acia *acia_;
  Adc *adc_;
  VideoUla *v_ula_;
  Crtc *crtc_;
};

#endif //M6502_SRC_BEEB_H_
