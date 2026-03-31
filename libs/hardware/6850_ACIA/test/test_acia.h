#pragma once

#include <gtest/gtest.h>
#include "acia_6850.h"
#include "bus.h"
#include <memory>
#include <vector>

// 6850 Status Register bit masks (mirrors SR_xxx_FLAG macros in acia_6850.cpp)
static constexpr uint8_t SR_RDRF = (1 << 0);  // Receive Data Register Full
static constexpr uint8_t SR_TDRE = (1 << 1);  // Transmit Data Register Empty
static constexpr uint8_t SR_DCD  = (1 << 2);  // Data Carrier Detect (latched)
static constexpr uint8_t SR_CTS  = (1 << 3);  // Clear To Send
static constexpr uint8_t SR_FE   = (1 << 4);  // Framing Error
static constexpr uint8_t SR_OVRN = (1 << 5);  // Overrun
static constexpr uint8_t SR_PE   = (1 << 6);  // Parity Error
static constexpr uint8_t SR_IRQ  = (1 << 7);  // Interrupt Request

class AciaTest : public ::testing::Test {
 protected:
  static constexpr uint16_t BASE = 0xfe08;

  std::shared_ptr<Bus>  bus_;
  std::shared_ptr<Acia> acia_;

  void SetUp() override;

  // ── MMIO helpers ────────────────────────────────────────────────────────────
  void     write_ctl(uint8_t data);
  void     write_tdr(uint8_t data);
  uint8_t  read_status();
  uint8_t  read_rdr();

  // ── Init helpers ─────────────────────────────────────────────────────────────
  // Perform the mandatory first master-reset and then configure.
  // Default: /1 clock, 8 data bits, 1 stop bit, no parity, no interrupts.
  // With CTS active (cassette selected).
  void init(uint8_t ctl = 0x14);

  // ── TX helpers ───────────────────────────────────────────────────────────────
  // Fire n tx_clock() ticks.
  void tx_clocks(int n);

  // Capture the bit sequence produced by transmitting one byte.
  // Returns bits in the order they appear on the wire (start, d0..dN, [parity], stop(s)).
  // Calls tx_clock() until the ACIA returns to IDLE after the frame.
  // Requires ACIA to be initialised and TDR to have been written.
  std::vector<bool> capture_tx_frame(int word_len, bool has_parity, int stop_bits);

  // ── RX helpers ───────────────────────────────────────────────────────────────
  // Drive one bit onto the RX data line and clock it in (once per bit for /1 mode).
  void rx_bit(bool bit, int clocks = 1);

  // Drive a complete async frame (start + data bits LSB first + stop) onto the RX line.
  void rx_byte(uint8_t data, int word_len = 8, int clocks_per_bit = 1);
};
