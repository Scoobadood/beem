//
// Created by Dave Durbin on 3/1/2023.
//

#include "acia_6850.h"

#include "spdlog/spdlog-inl.h"

const uint16_t WO_CTL = 0xfe08;

#define MASTER_RESET(c) ((c & 0x3) == 0x03)
#define CTR_DIV_SEL(c) (c & 0x3)
#define WORD_SEL(c) ((c >> 2) & 0x07)
#define TX_CTL_BITS(c) ((c >> 5) & 0x03)
#define RX_INT_ENBL(c) ((c & 0x80) == 0x80)

Acia::Acia() //
        : clk_divisor_{1} //
        , stop_bits_{2} //
        , word_length_{7} //
        , parity_{2} //
        , tx_int_enabled_{false} //
        , rx_int_enabled_{false} //
{}

void mmio_read(uint16_t addr, const std::shared_ptr<Bus>& bus) {
  spdlog::info("ACIA: Read from 0x{:04x}", addr);
  // FIXME: Temp hack to allow IRQ processing on other devices.
  if( addr == 0xfe08) bus->set_data(0x00);
}

void Acia::master_reset() {
  spdlog::info("ACIA: Master Reset");
}

void Acia::write_ctl(uint8_t data, const std::shared_ptr<Bus> &bus) {
  if (MASTER_RESET(data)) {
    master_reset();
  } else {
    clk_divisor_ = CTR_DIV_SEL(data);
    spdlog::info("ACIA: Set clock divisor to {}", clk_divisor_);
  }

  switch (WORD_SEL(data)) {
    case 0:
      word_length_ = 7;
      parity_ = 2;
      stop_bits_ = 2;
      break;
    case 1:
      word_length_ = 7;
      parity_ = 1;
      stop_bits_ = 2;
      break;
    case 2:
      word_length_ = 7;
      parity_ = 2;
      stop_bits_ = 1;
      break;
    case 3:
      word_length_ = 7;
      parity_ = 1;
      stop_bits_ = 1;
      break;
    case 4:
      word_length_ = 8;
      parity_ = 0;
      stop_bits_ = 2;
      break;
    case 5:
      word_length_ = 8;
      parity_ = 0;
      stop_bits_ = 1;
      break;
    case 6:
      word_length_ = 8;
      parity_ = 2;
      stop_bits_ = 1;
      break;
    case 7:
      word_length_ = 8;
      parity_ = 1;
      stop_bits_ = 1;
      break;
  }
  switch (TX_CTL_BITS(data)) {
    case 0:
      rts_default_low_ = true;
      tx_int_enabled_ = false;
      break;
    case 1:
      rts_default_low_ = true;
      tx_int_enabled_ = true;
      break;
    case 2:
      rts_default_low_ = false;
      tx_int_enabled_ = false;
    case 3:
      rts_default_low_ = true;
      tx_int_enabled_ = false;
  }
  rx_int_enabled_ = RX_INT_ENBL(data);

  spdlog::info("      Set Word length: {}, {} parity, {} stop bits",
               word_length_, parity_ == 0 ? "no" : (parity_ == 1 ? "odd" : "even"), stop_bits_
  );
  spdlog::info("      RTS: {}, TX interrupts {}enabled",
               rts_default_low_ ? "low" : "high",
               tx_int_enabled_ ? "" : "not "
  );
  spdlog::info("      RX interrupts {}enabled", rx_int_enabled_ ? "" : "not ");
}

void Acia::mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  auto data = bus->get_data();
  switch (addr) {
    case WO_CTL:
      write_ctl(data, bus);
      break;
    default:
      spdlog::info("ACIA: Write ({:02x}) to 0x{:04x}", bus->get_data(), addr);
      break;
  }
}

void Acia::tick(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < 0xfe08 || addr > 0xfe0f) return;
  auto read = bus->tst_RW();
  if (read) {
    mmio_read(addr, bus);
  } else {
    mmio_write(addr, bus);
  }
}
