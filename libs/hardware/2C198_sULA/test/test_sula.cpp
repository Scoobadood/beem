#include <gtest/gtest.h>
#include "2c198_sula.h"
#include "acia_6850.h"
#include "bus.h"
#include "i_cassette_player.h"
#include <memory>
#include <vector>

// ── Status register bit masks ────────────────────────────────────────────────
static constexpr uint8_t SR_DCD = (1 << 2);
static constexpr uint8_t SR_CTS = (1 << 3);

// ── Test addresses ───────────────────────────────────────────────────────────
static constexpr uint16_t ACIA_BASE = 0xFE08;
static constexpr uint16_t SULA_BASE = 0xFE10;

// ── Mock cassette player ─────────────────────────────────────────────────────
class MockCassettePlayer : public ICassettePlayer {
 public:
  // Configurable return values
  bool rx_data_value{true};       // default: mark (idle line)
  bool has_carrier_value{false};  // default: no carrier

  // Recorded calls
  int               rx_data_call_count{0};
  int               has_carrier_call_count{0};
  std::vector<bool> tx_bits_received;
  std::vector<bool> motor_calls;

  bool rx_data()    override { ++rx_data_call_count;    return rx_data_value;    }
  bool has_carrier() override { ++has_carrier_call_count; return has_carrier_value; }
  void tx_bit(bool bit) override { tx_bits_received.push_back(bit); }
  void set_motor(bool on) override { motor_calls.push_back(on); }
};

// ── Fixture ──────────────────────────────────────────────────────────────────
class SulaTest : public ::testing::Test {
 protected:
  void SetUp() override {
    bus_  = std::make_shared<Bus>();
    acia_ = std::make_shared<Acia>(ACIA_BASE);
    sula_ = std::make_unique<SerialUla>(SULA_BASE);
    sula_->set_acia(acia_);
    sula_->set_cassette_player(&player_);
  }

  // Write to the Serial ULA control register at SHEILA &FE10
  void write_scr(uint8_t val) {
    bus_->set_address(SULA_BASE);
    bus_->set_data(val);
    bus_->clr_RW();
    sula_->tick(bus_);
  }

  // Read the ACIA status register (at ACIA base addr, RW=1)
  uint8_t read_acia_status() {
    bus_->set_address(ACIA_BASE);
    bus_->set_RW();
    acia_->tick(bus_);
    return bus_->get_data();
  }

  // Write the ACIA control register
  void write_acia_ctl(uint8_t val) {
    bus_->set_address(ACIA_BASE);
    bus_->set_data(val);
    bus_->clr_RW();
    acia_->tick(bus_);
  }

  // Fire n 16 MHz clock ticks
  void tick_n(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
      sula_->tick_16mhz();
  }

  std::shared_ptr<Bus>       bus_;
  std::shared_ptr<Acia>      acia_;
  std::unique_ptr<SerialUla> sula_;
  MockCassettePlayer         player_;
};

// ════════════════════════════════════════════════════════════════════════════
// Group 1 — Clock generation (AUG 20.9.1 / 20.9.2)
//
// The sULA derives baud clocks by dividing the 16 MHz oscillator by 13×N,
// where N is the per-baud divider decoded from the SCR bits.
// ════════════════════════════════════════════════════════════════════════════

// SCR bits 0-2 = 000 → tx divider=1, period=13 ticks.
// After 13 ticks one tx_clock fires → player.tx_bit() called once.
TEST_F(SulaTest, tx_clock_rate_19200) {
  write_scr(0x00);  // tx bits 0-2=000, cassette selected
  ASSERT_EQ(player_.tx_bits_received.size(), 0u);
  tick_n(13);
  EXPECT_EQ(player_.tx_bits_received.size(), 1u);
  tick_n(13);
  EXPECT_EQ(player_.tx_bits_received.size(), 2u);
}

// SCR bits 0-2 = 001 → tx divider=2, period=26 ticks.
TEST_F(SulaTest, tx_clock_rate_9600) {
  write_scr(0x01);  // tx bits 0-2=001
  tick_n(25);
  EXPECT_EQ(player_.tx_bits_received.size(), 0u);
  tick_n(1);  // 26 total
  EXPECT_EQ(player_.tx_bits_received.size(), 1u);
}

// SCR bits 0-2 = 100 → tx divider=16, period=208 ticks.
TEST_F(SulaTest, tx_clock_rate_1200) {
  write_scr(0x04);  // tx bits 0-2=100
  tick_n(207);
  EXPECT_EQ(player_.tx_bits_received.size(), 0u);
  tick_n(1);  // 208 total
  EXPECT_EQ(player_.tx_bits_received.size(), 1u);
}

// SCR bits 3-5 = 000 → rx divider=1, period=13 ticks.
TEST_F(SulaTest, rx_clock_rate_19200) {
  write_scr(0x00);
  ASSERT_EQ(player_.rx_data_call_count, 0);
  tick_n(13);
  EXPECT_EQ(player_.rx_data_call_count, 1);
  tick_n(13);
  EXPECT_EQ(player_.rx_data_call_count, 2);
}

// SCR bits 3-5 = 100 (bit field = 0x04 << 3 = 0x20) → rx divider=16, period=208.
TEST_F(SulaTest, rx_clock_rate_1200) {
  write_scr(0x20);  // rx bits 3-5=100
  tick_n(207);
  EXPECT_EQ(player_.rx_data_call_count, 0);
  tick_n(1);  // 208 total
  EXPECT_EQ(player_.rx_data_call_count, 1);
}

// TX and RX clocks are derived independently from bits 0-2 and 3-5.
// SCR=0x21: tx bits 0-2=001 (divider=2, period=26), rx bits 3-5=100 (divider=16, period=208).
TEST_F(SulaTest, tx_rx_clocks_independent) {
  write_scr(0x21);
  tick_n(26);
  EXPECT_EQ(player_.tx_bits_received.size(), 1u);  // tx fired
  EXPECT_EQ(player_.rx_data_call_count, 0);         // rx not yet

  tick_n(208 - 26);  // 208 ticks total
  EXPECT_EQ(player_.rx_data_call_count, 1);         // rx fired once
  EXPECT_EQ(player_.tx_bits_received.size(), 8u);   // tx fired 208/26=8 times
}

// ════════════════════════════════════════════════════════════════════════════
// Group 2 — CTS control (AUG 20.9.3)
//
// SCR bit 6 = 0 → cassette selected → ACIA CTS cleared (active-low).
// SCR bit 6 = 1 → RS423 selected   → ACIA CTS raised  (inactive).
// ════════════════════════════════════════════════════════════════════════════

TEST_F(SulaTest, cts_active_when_cassette_selected) {
  write_acia_ctl(0x03);  // release ACIA power-on reset
  write_scr(0x00);       // bit 6=0: cassette
  EXPECT_FALSE(read_acia_status() & SR_CTS);
}

TEST_F(SulaTest, cts_inactive_when_rs423_selected) {
  write_acia_ctl(0x03);
  write_scr(0x40);  // bit 6=1: RS423
  EXPECT_TRUE(read_acia_status() & SR_CTS);
}

TEST_F(SulaTest, switching_cassette_to_rs423_raises_cts) {
  write_acia_ctl(0x03);
  write_scr(0x00);
  ASSERT_FALSE(read_acia_status() & SR_CTS);
  write_scr(0x40);
  EXPECT_TRUE(read_acia_status() & SR_CTS);
}

TEST_F(SulaTest, switching_rs423_to_cassette_clears_cts) {
  write_acia_ctl(0x03);
  write_scr(0x40);
  ASSERT_TRUE(read_acia_status() & SR_CTS);
  write_scr(0x00);
  EXPECT_FALSE(read_acia_status() & SR_CTS);
}

// ════════════════════════════════════════════════════════════════════════════
// Group 3 — Motor control (AUG 20.9.4)
// ════════════════════════════════════════════════════════════════════════════

TEST_F(SulaTest, motor_on_when_scr_bit7_set) {
  write_scr(0x80);
  EXPECT_TRUE(sula_->is_motor_on());
  ASSERT_FALSE(player_.motor_calls.empty());
  EXPECT_TRUE(player_.motor_calls.back());
}

TEST_F(SulaTest, motor_off_when_scr_bit7_clear) {
  write_scr(0x80);
  player_.motor_calls.clear();
  write_scr(0x00);
  EXPECT_FALSE(sula_->is_motor_on());
  ASSERT_FALSE(player_.motor_calls.empty());
  EXPECT_FALSE(player_.motor_calls.back());
}

TEST_F(SulaTest, motor_state_accessible_via_accessor) {
  write_scr(0xC0);
  EXPECT_TRUE(sula_->is_motor_on());
  write_scr(0x40);
  EXPECT_FALSE(sula_->is_motor_on());
}

// ════════════════════════════════════════════════════════════════════════════
// Group 4 — Cassette player RX wiring
// ════════════════════════════════════════════════════════════════════════════

TEST_F(SulaTest, rx_data_polled_each_rx_tick) {
  write_scr(0x00);
  tick_n(13);
  EXPECT_EQ(player_.rx_data_call_count, 1);
  tick_n(13);
  EXPECT_EQ(player_.rx_data_call_count, 2);
}

TEST_F(SulaTest, has_carrier_polled_each_rx_tick) {
  write_scr(0x00);
  tick_n(13);
  EXPECT_EQ(player_.has_carrier_call_count, 1);
  tick_n(13);
  EXPECT_EQ(player_.has_carrier_call_count, 2);
}

// DCD (active-low) is raised in ACIA status when no carrier is detected.
// The 6850 latches DCD loss: once raised it stays set in status until
// status register is read followed by an RDR read.
TEST_F(SulaTest, acia_dcd_set_when_no_carrier_detected) {
  player_.has_carrier_value = false;  // no carrier
  write_scr(0x00);
  tick_n(13);  // sULA calls acia_->raise_dcd()
  EXPECT_TRUE(read_acia_status() & SR_DCD);
}

// After the 6850 DCD latch is set, reading status then reading RDR clears it.
// This tests that the sULA's call to clear_dcd() when carrier returns allows
// the latch to be cleared through the standard 6850 acknowledge sequence.
TEST_F(SulaTest, acia_dcd_latch_clears_after_status_and_rdr_read) {
  // Raise the latch
  player_.has_carrier_value = false;
  write_scr(0x00);
  tick_n(13);
  ASSERT_TRUE(read_acia_status() & SR_DCD);

  // read_acia_status() above advanced the latch state (sr2_high_wait_for_data_read_).
  // Reading RDR (ACIA base+1) now clears SR_DCD.
  bus_->set_address(ACIA_BASE + 1);
  bus_->set_RW();
  acia_->tick(bus_);

  EXPECT_FALSE(read_acia_status() & SR_DCD);
}

// ════════════════════════════════════════════════════════════════════════════
// Group 5 — Cassette player TX wiring
// ════════════════════════════════════════════════════════════════════════════

// While the ACIA TX line is idle (mark), player receives true each tx tick.
TEST_F(SulaTest, tx_player_receives_mark_when_acia_idle) {
  write_scr(0x00);  // 19200 tx, cassette selected
  tick_n(13);
  ASSERT_EQ(player_.tx_bits_received.size(), 1u);
  EXPECT_TRUE(player_.tx_bits_received[0]);  // mark = 1
}

// Player is called once per tx tick regardless of ACIA state.
TEST_F(SulaTest, tx_player_called_once_per_tx_tick) {
  write_scr(0x00);
  tick_n(26);  // two tx clock ticks at 19200 baud
  EXPECT_EQ(player_.tx_bits_received.size(), 2u);
}

// ════════════════════════════════════════════════════════════════════════════
// Group 6 — RS423 isolation
//
// When RS423 is selected (SCR bit 6 = 1), the cassette player must NOT
// be called for rx data, DCD, or tx — only motor control remains active.
// ════════════════════════════════════════════════════════════════════════════

TEST_F(SulaTest, cassette_player_rx_not_called_when_rs423_selected) {
  write_scr(0x40);  // RS423 selected
  tick_n(13 * 4);   // fire several clock periods
  EXPECT_EQ(player_.rx_data_call_count, 0);
  EXPECT_EQ(player_.has_carrier_call_count, 0);
}

TEST_F(SulaTest, cassette_player_tx_not_called_when_rs423_selected) {
  write_scr(0x40);
  tick_n(13 * 4);
  EXPECT_EQ(player_.tx_bits_received.size(), 0u);
}

TEST_F(SulaTest, motor_called_regardless_of_rs423_selection) {
  write_scr(0xC0);  // RS423 (bit 6=1) + motor on (bit 7=1)
  ASSERT_FALSE(player_.motor_calls.empty());
  EXPECT_TRUE(player_.motor_calls.back());
}

// ════════════════════════════════════════════════════════════════════════════
// Group 7 — Safety / edge cases
// ════════════════════════════════════════════════════════════════════════════

TEST_F(SulaTest, no_crash_without_player_rx_tick) {
  sula_->set_cassette_player(nullptr);
  write_scr(0x00);
  EXPECT_NO_FATAL_FAILURE(tick_n(13));
}

TEST_F(SulaTest, no_crash_without_player_tx_tick) {
  sula_->set_cassette_player(nullptr);
  write_scr(0x00);
  EXPECT_NO_FATAL_FAILURE(tick_n(13));
}

TEST_F(SulaTest, no_crash_without_player_scr_write) {
  sula_->set_cassette_player(nullptr);
  EXPECT_NO_FATAL_FAILURE(write_scr(0x80));
}

TEST_F(SulaTest, no_crash_without_acia) {
  sula_->set_acia(nullptr);
  write_scr(0x00);
  EXPECT_NO_FATAL_FAILURE(tick_n(13));
}
