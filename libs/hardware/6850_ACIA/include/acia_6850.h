//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef BEEB_HARDWARE_ACIA_6850_H_
#define BEEB_HARDWARE_ACIA_6850_H_

#include "bus.h"
#include "data_connectors.h"
#include <cstdint>
#include <spdlog/spdlog.h>

class Acia {
 public:
  explicit Acia(uint16_t base_addr);

  void tick(const std::shared_ptr<Bus> &bus);
  void tx_clock();
  void rx_clock();
  // IRQ is active low
  [[nodiscard]] inline bool has_irq() const { return !irq_; }

  void clear_cts();
  void raise_cts();
  void clear_dcd();
  void raise_dcd();

 private:
  void perform_master_reset();
  static uint8_t clock_divisor(uint8_t ctl);
  void configure_serial_protocol(uint8_t data);
  void enable_tx_interrupts();
  void disable_tx_interrupts();
  void enable_rx_interrupts();
  void disable_rx_interrupts();
  void maybe_rw(const std::shared_ptr<Bus> &bus);
  void shift_out_data();
  void maybe_load_shift_register();

  void set_output(uint8_t out);
  void raise_interrupt();
  void clear_interrupt();
  void tdr_went_empty();
  void cts_went_active_low();
  void cts_went_inactive_high();
  void dcd_went_active_low();
  void dcd_went_inactive_high();

  void mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus);
  void read_rdr(const std::shared_ptr<Bus> &bus);
  void read_status(const std::shared_ptr<Bus> &bus);

  void mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus);
  void write_tdr(uint8_t data);

  bool is_in_power_on_reset_;

  /* Control register */
  uint32_t tx_clock_ticks_;
  uint8_t clk_divisor_;
  uint8_t stop_bits_;
  uint8_t word_length_;
  uint8_t parity_; // 0 = none, 1 == odd, 2 == even
  bool tx_int_enabled_;
  bool rx_int_enabled_;

  /*
   * Transmitting data
   */
  bool rts_;              // Active low

  bool tdr_is_full_;
  uint8_t tx_shift_count_;
  uint8_t parity_bit_;
  uint8_t out_;
  enum TransmitState {
    IDLE = 0,
    SEND_START_BIT = 1,
    SEND_BITS = 2,
    SEND_PARITY = 3,
    SEND_STOP_BIT_1 = 4,
    SEND_STOP_BIT_2 = 5
  } state_;

  /*
   * Receiving data
   */
  bool rdr_is_full_;
  bool rdr_was_read_;
  bool parity_error_;
  bool overrun_error_pending_;
  bool overrun_error_;
  bool sr2_high_wait_for_sr_read_;
  bool sr2_high_wait_for_data_read_;

  /* IRQ */
  bool irq_;
 protected:
  void write_ctl(uint8_t data);

  /* Status register */
  uint8_t status_register_;
  uint8_t control_register_;
  uint8_t tdr_;
  uint8_t rdr_;
  uint8_t tx_shift_register_;
  uint8_t rx_shift_register_;
  uint16_t base_addr_;
  bool cts_;

  bool dcd_;
  std::shared_ptr<spdlog::logger> logger_;
};

#endif // BEEB_HARDWARE_ACIA_6850_H_
