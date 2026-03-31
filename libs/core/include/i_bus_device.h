/**
 * Common interface for devices that live on the SHEILA bus (0xfe00–0xfeff).
 *
 * Beeb holds a vector of IBusDevice* and uses it for three things:
 *   1. SHEILA MMIO dispatch — when the CPU addresses SHEILA, find the device
 *      that decodes the address and call tick() on it.
 *   2. Cycle-stretch detection — any access to a 1MHz device stretches the
 *      2MHz CPU clock to align with the 1MHz bus.
 *   3. IRQ aggregation — collect /IRQ from all devices before each CPU tick.
 *
 * Devices that communicate only via VIA parallel ports (SN76489, Keyboard)
 * are not IBusDevice; they are wired and ticked explicitly.
 *
 * VideoUla is also excluded: its MMIO writes are handled inline within its
 * 16MHz pixel-generation tick(), which requires both the CPU bus and the DRAM
 * bus. The IBusDevice interface cannot express that dual-bus signature.
 */
#ifndef BEEB_HARDWARE_I_BUS_DEVICE_H_
#define BEEB_HARDWARE_I_BUS_DEVICE_H_

#include "bus.h"
#include <memory>
#include <cstdint>

class IBusDevice {
public:
  virtual ~IBusDevice() = default;

  /**
   * Called when the CPU is in SHEILA and decodes() returned true for the
   * current bus address. The device reads or writes the bus as appropriate.
   */
  virtual void tick(const std::shared_ptr<Bus>& bus) = 0;

  /**
   * Returns true if this device owns the given address within SHEILA.
   * Used for dispatch and cycle-stretch detection.
   */
  virtual bool decodes(uint16_t addr) const = 0;

  /**
   * Returns true if accessing this device requires cycle-stretching
   * (i.e. the device runs on the 1MHz bus, not the 2MHz bus).
   */
  virtual bool is_1mhz_device() const = 0;

  /**
   * Returns true if this device is currently asserting /IRQ to the CPU.
   * Default is false; override in devices that have an IRQ output.
   */
  virtual bool has_irq() const { return false; }
};

#endif // BEEB_HARDWARE_I_BUS_DEVICE_H_
