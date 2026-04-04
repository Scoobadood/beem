#include "2c198_sula.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "data_connectors.h"

#include <spdlog/spdlog.h>
#include <fstream>

// SULA
// fe10 but also all the way to fe1f
const uint16_t WO_SCR = 0;

SerialUla::SerialUla(uint16_t base_addr)
  : base_addr_{base_addr} //
{
  spdlog::drop("sULA");
  try {
    auto logger = spdlog::basic_logger_mt("sULA", "logs/sULA.txt", true);
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    logger->info("SerialUla constructed at base {:04x}", base_addr);
  } catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
}

void
SerialUla::maybe_rw(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < 0xfe10 || addr > 0xfe1f) return;
  auto read = bus->tst_RW();
  if (read) {
    spdlog::error("sULA: Unsupported attempt to read Serial ULA read from {:04x}", addr);
  } else {
    mmio_write(addr, bus);
  }
}

void
SerialUla::tick(const std::shared_ptr<Bus> &bus) {
  maybe_rw(bus);
}

void SerialUla::tick_16mhz() {
  maybe_tx_clock_tick();
  maybe_rx_clock_tick();
}

/*
 * +------+-----------+
 * | Bits | Baud Rate |
 * +------+-----------+
 * |  111 | 75        |
 * |  110 | 150       |
 * |  101 | 300       |
 * |  100 | 1200      |
 * |  011 | 2400      |
 * |  010 | 4800      |
 * |  001 | 9600      |
 * |  000 | 19200     |
 * +------+-----------+
 */
void decode_clock_bits(uint8_t bits, uint16_t &baud, uint16_t &clock) {
  bits = bits & 0x07;
  switch (bits) {
    case 0:
      baud = 19200;
      clock = 1;
      break;
    case 1:
      baud = 9600;
      clock = 2;
      break;
    case 2:
      baud = 4800;
      clock = 4;
      break;
    case 3:
      baud = 2400;
      clock = 8;
      break;
    case 4:
      baud = 1200;
      clock = 16;
      break;
    case 5:
      baud = 300;
      clock = 64;
      break;
    case 6:
      baud = 150;
      clock = 128;
      break;
    case 7:
      baud = 75;
      clock = 256;
      break;
  }
}

void
SerialUla::mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  if (addr != base_addr_) {
    spdlog::error("Attempt to write to sULA on bad address {}", addr);
    return;
  }

  auto data = bus->get_data();
  auto old_motor_state = serial_control_register_ & 0x80;
  serial_control_register_ = data;
  auto new_motor_state = serial_control_register_ & 0x80;
  auto motor_state_changed = (old_motor_state != new_motor_state);
  /*
   * These define the transmit baud rate so that 000 generates 19200 baud and 111 generates
   * 75 baud. Note that this relies upon the 6850 control register being set to divide the
   * incoming clock signal by 64. *FX8 is used to select the transmit baud rate on an RS423 input.
   */
  decode_clock_bits(data & 0x07, tx_baud_, tx_clock_divider_);
  tx_clock_counter_ = 13 * tx_clock_divider_;
  decode_clock_bits((data >> 3) & 0x07, rx_baud_, rx_clock_divider_);
  // In cassette mode the FSK decoder generates the RX clock at a fixed ~19,200 Hz
  // rate (divider=64) regardless of SCR bits 3-5. The programmable RX divider only
  // applies to RS423 mode. (AUG 20.6.1: "the serial ULA is always set to 300 baud
  // for cassette, so division by 64 actually generates 300 baud.")
  rx_clock_counter_ = 13 * effective_rx_divider();

  spdlog::get("sULA")->info("Serial Control Written. CM: {}, Sel: {}, Rx: {}, Tx: {}",
                            (data & 0x80) ? "on" : "off",
                            (data & 0x40) ? "rs423" : "cass",
                            rx_baud_,
                            tx_baud_
  );

  // Cassette selected. Take CTS low
  if (acia_) {
    if ((data & 0x40) == 0) {
      acia_->clear_cts();
    } else {
      acia_->raise_cts();
    }
  }

  if (cassette_port_ && motor_state_changed) {
    cassette_port_->set_motor(new_motor_state);
    if (acia_ && !is_rs423_selected()) {
      if (new_motor_state == 0) {
        // Motor off: no tape movement = no carrier. DCD* HIGH = receiver disabled.
        acia_->apply_carrier();
        spdlog::get("sULA")->info("Motor off");
      } else {
        if (cassette_port_->has_carrier()) {
          // Carrier present: DCD* LOW = receiver enabled (active-low convention).
          spdlog::get("sULA")->info("Motor on, carrier detected");
          acia_->drop_carrier();
        } else {
          spdlog::get("sULA")->info("Motor on, no carrier");
        }
      }
    }
  }
}

uint8_t
SerialUla::serial_control_register() const {
  return serial_control_register_;
}
bool
SerialUla::is_motor_on() const {
  return ((serial_control_register_ & 0x80) == 0x80);
}

uint16_t
SerialUla::transmit_baud() const {
  return tx_baud_;
}

uint16_t
SerialUla::receive_baud() const {
  return rx_baud_;
}

void SerialUla::maybe_tx_clock_tick() {
  if (--tx_clock_counter_ != 0) return;
  tx_clock_counter_ = 13 * tx_clock_divider_;
  if (acia_) {
    acia_->tx_clock();
    if (cassette_port_ && !is_rs423_selected())
      cassette_port_->tx_bit(acia_->tx_pin());
  }
}

void SerialUla::maybe_rx_clock_tick() {
  if (--rx_clock_counter_ != 0) return;
  rx_clock_counter_ = 13 * effective_rx_divider();

  // TODO: Handle RS423 later.
  if (!acia_ || is_rs423_selected() || !cassette_port_) {
    return;
  }

  // The tape stream runs at the baud rate, not the ACIA external clock rate.
  // Advance the tape only once per clk_divisor rx_clock() ticks so each
  // bit stays stable for the full bit period (required by ACIA start-bit
  // false-deletion: needs clk_divisor/2 consecutive matching samples).
  if (++rx_tape_counter_ >= (uint32_t) acia_->clk_divisor()) {
    rx_tape_counter_ = 0;
    current_rx_bit_ = cassette_port_->rx_data();

    // DCD* is active-low: carrier present → DCD* LOW → clear_dcd().
    // No carrier (gap) → DCD* HIGH → raise_dcd(). AUG §14.2.5 / AUG p.445.
    if (cassette_port_->has_carrier())
      acia_->drop_carrier();
    else
      acia_->apply_carrier();
  }
  acia_->set_rx_data(current_rx_bit_);
  acia_->rx_clock();
}

bool SerialUla::is_rs423_selected() const {
  return (serial_control_register_ & 0x40) != 0;
}

uint16_t SerialUla::effective_rx_divider() const {
  // In cassette mode the FSK decoder generates the RX clock at a fixed rate
  // equivalent to divider=64, regardless of SCR bits 3-5.
  return is_rs423_selected() ? rx_clock_divider_ : uint16_t{64};
}

void SerialUla::set_acia(const std::shared_ptr<Acia> &acia) {
  acia_ = acia;
}

void SerialUla::set_cassette_port(ICassettePort *port) {
  cassette_port_ = port;
}
