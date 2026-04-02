/**
 * The serial ULA is a custom Integrated Circuit (IC) developed by Acorn Computer for use in its
 * BBC Microcomputer series. It shares the 6850 ACIA between the cassette port and serial port,
 * provides a clock for the ACIA, and modulation, demodulation and motor control for the cassette
 * port.
 *
 * The original IC was fabricated by Ferranti as an Uncommitted Logic Array (ULA), permanently
 * mask-programmed to Acorn's specification; part number ULA 2C199E. Later, Acorn second-sourced
 * the chip from VLSI Technology, Inc.; this was a different design, more correctly known as the
 * SERPROC (Serial Processor); part number VC 2026, Acorn part number 201,648. With one exception
 * this was functionally and physically compatible with the serial ULA.
 */
#ifndef BEEB_HARDWARE_2C198_SULA_H
#define BEEB_HARDWARE_2C198_SULA_H

#include <set>

#include "bus.h"
#include "data_connectors.h"
#include "acia_6850.h"
#include "i_bus_device.h"
#include "i_cassette_port.h"

class AbstractSula {
 public :
  AbstractSula() = default;
  virtual ~AbstractSula() = default;
};

class SerialUla : public AbstractSula, public IBusDevice {
 public:
  explicit SerialUla(uint16_t base_addr);
  ~SerialUla() override = default;

  void tick(const std::shared_ptr<Bus>& bus) override;
  [[nodiscard]] bool decodes(uint16_t addr) const override { return addr >= base_addr_ && addr <= base_addr_ + 0x0f; }
  [[nodiscard]] bool is_1mhz_device() const override { return true; }

  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);
  void tick_16mhz();

  void set_acia(const std::shared_ptr<Acia>& acia);
  void set_cassette_port(ICassettePort* port);

  uint8_t serial_control_register() const;
  bool is_motor_on() const;
  uint16_t transmit_baud() const;
  uint16_t receive_baud() const;

 private:
  void maybe_rw(const std::shared_ptr<Bus> &bus);
  void maybe_tx_clock_tick();
  void maybe_rx_clock_tick();
  [[nodiscard]] bool is_rs423_selected() const;
  [[nodiscard]] uint16_t effective_rx_divider() const;

  uint16_t tx_baud_;
  uint16_t rx_baud_;

  uint16_t base_addr_;
  uint8_t serial_control_register_;
  std::set<data_subscriber_8_bit_ptr> carrier_detect_subscribers_;

  uint32_t tx_clock_counter_;
  uint16_t tx_clock_divider_;

  uint32_t rx_clock_counter_;
  uint16_t rx_clock_divider_;

  // Tape bit buffer: next_bit() is called once per baud period (every
  // clk_divisor rx_clock() ticks) and the result held stable between advances.
  uint32_t rx_tape_counter_{0};
  bool current_rx_bit_{true};  // idle / mark

  std::shared_ptr<Acia> acia_;
  ICassettePort* cassette_port_{nullptr};
};

#endif // BEEB_HARDWARE_2C198_SULA_H