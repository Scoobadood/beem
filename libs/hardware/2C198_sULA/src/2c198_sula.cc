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
  try {
    auto logger = spdlog::basic_logger_mt("sULA", "logs/sULA.txt", true);
    logger->flush_on(spdlog::level::trace);
  }
  catch (const spdlog::spdlog_ex &ex) {
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

  // Check for carrier detect
  if (acia_) {
    /* TODO: Hacky override to simulate the DCD - disabled in case FS calls are slowing shiz down.*/
//    std::ifstream dcd("dcd");
//    if (dcd.good()) {
//      acia_->raise_dcd();
//    } else {
//      acia_->clear_dcd();
//    }
  }
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
  serial_control_register_ = data;
  /*
   * These define the transmit baud rate so that 000 generates 19200 baud and 111 generates
   * 75 baud. Note that this relies upon the 6850 control register being set to divide the
   * incoming clock signal by 64. *FX8 is used to select the transmit baud rate on an RS423 input.
   */
  decode_clock_bits(data & 0x07, tx_baud_, tx_clock_divider_);
  tx_clock_counter_ = 13 * tx_clock_divider_;
  decode_clock_bits((data >> 3) & 0x07, rx_baud_, rx_clock_divider_);
  rx_clock_counter_ = 13 * rx_clock_divider_;

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

  if (cassette_player_)
    cassette_player_->set_motor(is_motor_on());
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
    if (cassette_player_ && !is_rs423_selected())
      cassette_player_->tx_bit(acia_->tx_pin());
  }
}

void SerialUla::maybe_rx_clock_tick() {
  if (--rx_clock_counter_ != 0) return;
  rx_clock_counter_ = 13 * rx_clock_divider_;
  if (acia_) {
    if (cassette_player_ && !is_rs423_selected()) {
      acia_->set_rx_data(cassette_player_->rx_data());
      if (cassette_player_->has_carrier())
        acia_->clear_dcd();
      else
        acia_->raise_dcd();
    }
    acia_->rx_clock();
  }
}

bool SerialUla::is_rs423_selected() const {
  return (serial_control_register_ & 0x40) != 0;
}

void SerialUla::set_acia(const std::shared_ptr<Acia> &acia) {
  acia_ = acia;
}

void SerialUla::set_cassette_player(ICassettePlayer* player) {
  cassette_player_ = player;
}
