//
// Created by Dave Durbin on 13/1/2023.
//

#ifndef BEEB_HARDWARE_ADC_H_
#define BEEB_HARDWARE_ADC_H_

#include "bus.h"
#include "i_bus_device.h"

#include <cstdint>

class Adc : public IBusDevice {
 public:
  explicit Adc(uint16_t base_address);

  void tick(const std::shared_ptr<Bus>& bus) override;
  [[nodiscard]] bool decodes(uint16_t addr) const override { return addr >= base_address_ && addr <= base_address_ + 2; }
  [[nodiscard]] bool is_1mhz_device() const override { return true; }

 private:
  uint16_t base_address_;
  uint8_t channel_;
  uint8_t mode_;
};

#endif // BEEB_HARDWARE_ADC_H_
