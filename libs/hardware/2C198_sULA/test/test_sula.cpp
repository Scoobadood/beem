#include <gtest/gtest.h>
#include "2c198_sula.h"
#include "acia_6850.h"
#include "bus.h"
#include "i_cassette_port.h"
#include <memory>
#include <vector>

// ── Status register bit masks ────────────────────────────────────────────────
static constexpr uint8_t SR_DCD = (1 << 2);
static constexpr uint8_t SR_CTS = (1 << 3);

// ── Test addresses ───────────────────────────────────────────────────────────
static constexpr uint16_t ACIA_BASE = 0xFE08;
static constexpr uint16_t SULA_BASE = 0xFE10;

// ── Mock cassette player ─────────────────────────────────────────────────────
class MockCassettePort : public ICassettePort {
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
    sula_->set_cassette_port(&player_);
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

  // Fire n 16 MHz clock ticks (public so helpers can call it)
 public:
  void tick_n(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
      sula_->tick_16mhz();
  }

  std::shared_ptr<Bus>       bus_;
  std::shared_ptr<Acia>      acia_;
  std::unique_ptr<SerialUla> sula_;
  MockCassettePort         player_;
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

// In cassette mode the FSK decoder runs at a fixed period of 832 ticks (13×64)
// regardless of SCR bits 3-5.  SCR=0x00 → bits 3-5=000 (would be 19200), but
// cassette overrides to the fixed 832-tick period.
TEST_F(SulaTest, rx_clock_rate_19200) {
  write_scr(0x00);
  ASSERT_EQ(player_.rx_data_call_count, 0);
  tick_n(832);
  EXPECT_EQ(player_.rx_data_call_count, 1);
  tick_n(832);
  EXPECT_EQ(player_.rx_data_call_count, 2);
}

// Cassette mode ignores SCR bits 3-5: even with bits set to 1200 baud (divider=16),
// the period stays fixed at 832 ticks.
TEST_F(SulaTest, rx_clock_rate_cassette_always_fixed) {
  write_scr(0x20);  // rx bits 3-5=100 (would be 1200 baud/divider=16 in RS423)
  tick_n(831);
  EXPECT_EQ(player_.rx_data_call_count, 0);
  tick_n(1);  // 832 total
  EXPECT_EQ(player_.rx_data_call_count, 1);
}

// TX and RX clocks are derived independently.
// SCR=0x21: tx bits 0-2=001 (divider=2, period=26 ticks),
//            rx bits 3-5=100 (cassette fixed period=832 ticks).
TEST_F(SulaTest, tx_rx_clocks_independent) {
  write_scr(0x21);
  tick_n(26);
  EXPECT_EQ(player_.tx_bits_received.size(), 1u);  // tx fired
  EXPECT_EQ(player_.rx_data_call_count, 0);         // rx not yet (needs 832 ticks)

  tick_n(832 - 26);  // 832 ticks total
  EXPECT_EQ(player_.rx_data_call_count, 1);         // rx fired once
  EXPECT_EQ(player_.tx_bits_received.size(), 32u);  // tx fired 832/26=32 times
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
  tick_n(832);
  EXPECT_EQ(player_.rx_data_call_count, 1);
  tick_n(832);
  EXPECT_EQ(player_.rx_data_call_count, 2);
}

TEST_F(SulaTest, has_carrier_polled_each_rx_tick) {
  write_scr(0x00);
  player_.has_carrier_call_count = 0;  // reset: write_scr also calls has_carrier once
  tick_n(832);
  EXPECT_EQ(player_.has_carrier_call_count, 1);
  tick_n(832);
  EXPECT_EQ(player_.has_carrier_call_count, 2);
}

// SR_DCD is clear when no carrier is detected (BBC: carrier present = SR_DCD=1).
TEST_F(SulaTest, acia_dcd_clear_when_no_carrier_detected) {
  player_.has_carrier_value = false;  // no carrier
  write_scr(0x00);
  tick_n(832);
  EXPECT_FALSE(read_acia_status() & SR_DCD);
}

// SR_DCD is set when carrier is detected.
TEST_F(SulaTest, acia_dcd_set_when_carrier_detected) {
  player_.has_carrier_value = true;  // carrier present
  write_scr(0x00);
  tick_n(832);  // sULA calls acia_->raise_dcd()
  EXPECT_TRUE(read_acia_status() & SR_DCD);
}

// After the 6850 DCD latch is set (carrier arrives then leaves), reading status
// then reading RDR clears it.
TEST_F(SulaTest, acia_dcd_latch_clears_after_status_and_rdr_read) {
  write_scr(0x00);

  // Carrier present — raise_dcd() arms the latch (SR_DCD=1).
  player_.has_carrier_value = true;
  tick_n(832);

  // Carrier ends — clear_dcd() called; latch still pending so SR_DCD stays 1.
  player_.has_carrier_value = false;
  tick_n(832);
  ASSERT_TRUE(read_acia_status() & SR_DCD);

  // read_acia_status() above advanced the latch to wait-for-data-read.
  // Reading RDR (ACIA base+1) now clears SR_DCD (dcd_ is false = no carrier).
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
  sula_->set_cassette_port(nullptr);
  write_scr(0x00);
  EXPECT_NO_FATAL_FAILURE(tick_n(13));
}

TEST_F(SulaTest, no_crash_without_player_tx_tick) {
  sula_->set_cassette_port(nullptr);
  write_scr(0x00);
  EXPECT_NO_FATAL_FAILURE(tick_n(13));
}

TEST_F(SulaTest, no_crash_without_player_scr_write) {
  sula_->set_cassette_port(nullptr);
  EXPECT_NO_FATAL_FAILURE(write_scr(0x80));
}

TEST_F(SulaTest, no_crash_without_acia) {
  sula_->set_acia(nullptr);
  write_scr(0x00);
  EXPECT_NO_FATAL_FAILURE(tick_n(13));
}

// ════════════════════════════════════════════════════════════════════════════
// Group 8 — End-to-end 1200 baud RX
//
// Feed a known byte through the full sULA+ACIA chain at 1200 baud (÷64)
// and verify the ACIA receives the correct value.
// ════════════════════════════════════════════════════════════════════════════

// A cassette port that returns bits from a pre-loaded sequence.
struct SeqPort : ICassettePort {
  std::vector<bool> bits;
  size_t idx{0};
  bool rx_data() override { return idx < bits.size() ? bits[idx++] : true; }
  bool has_carrier() override { return true; }
  void tx_bit(bool) override {}
  void set_motor(bool) override {}
};

// Feed a single 8N1 frame for the given byte preceded by carrier_count mark
// bits through the sULA+ACIA chain at 1200 baud and return the byte the ACIA
// put into its RDR.  Returns 0xFF and fails the test if RDRF is not set.
static uint8_t receive_byte_1200baud(
    SulaTest& t,
    std::shared_ptr<Bus>& bus,
    std::shared_ptr<Acia>& acia,
    std::unique_ptr<SerialUla>& sula,
    uint8_t byte_val,
    uint32_t carrier_count = 2)
{
  SeqPort seq;

  // Build bit sequence: N carrier (mark) + 8N1 frame
  for (uint32_t i = 0; i < carrier_count; ++i) seq.bits.push_back(true);
  seq.bits.push_back(false); // start bit
  for (int i = 0; i < 8; ++i) seq.bits.push_back((byte_val >> i) & 1);
  seq.bits.push_back(true);  // stop bit

  sula->set_cassette_port(&seq);

  // Configure ACIA: master reset, then ÷16 8N1 no interrupts.
  // Cassette sULA feeds a fixed ~19200 Hz RX clock; ACIA ÷16 gives 1200 baud.
  bus->set_address(0xFE08); bus->set_data(0x03); bus->clr_RW(); acia->tick(bus);
  acia->clear_cts();
  bus->set_address(0xFE08); bus->set_data(0x55); bus->clr_RW(); acia->tick(bus);

  // Configure sULA: cassette, motor on (bit 7). SCR rx bits are ignored in
  // cassette mode — the fixed period of 832 ticks always applies.
  bus->set_address(0xFE10); bus->set_data(0x80); bus->clr_RW(); sula->tick(bus);

  // Run: (carrier_count + 10) tape advances; each = 16 rx_clocks * 832 16MHz ticks.
  // Add one extra bit period for the stop bit to be sampled.
  const uint32_t ticks_per_bit = 16u * 832u; // = 13312
  const uint32_t total_bits = carrier_count + 10u; // carrier + start + 8 data + stop
  t.tick_n(total_bits * ticks_per_bit + ticks_per_bit);

  // Check RDRF (status bit 0)
  bus->set_address(0xFE08); bus->set_RW(); acia->tick(bus);
  uint8_t status = bus->get_data();
  EXPECT_TRUE(status & 0x01) << "RDRF not set after receiving frame";
  if (!(status & 0x01)) return 0xFF;

  // Read RDR
  bus->set_address(0xFE09); bus->set_RW(); acia->tick(bus);
  return bus->get_data();
}

TEST_F(SulaTest, rx_1200_baud_receives_0x42) {
  EXPECT_EQ(0x42u, receive_byte_1200baud(*this, bus_, acia_, sula_, 0x42));
}

TEST_F(SulaTest, rx_1200_baud_receives_0x2A) {
  EXPECT_EQ(0x2Au, receive_byte_1200baud(*this, bus_, acia_, sula_, 0x2A));
}

TEST_F(SulaTest, rx_1200_baud_receives_0xFF) {
  EXPECT_EQ(0xFFu, receive_byte_1200baud(*this, bus_, acia_, sula_, 0xFF));
}

TEST_F(SulaTest, rx_1200_baud_receives_0x00) {
  EXPECT_EQ(0x00u, receive_byte_1200baud(*this, bus_, acia_, sula_, 0x00));
}

// ════════════════════════════════════════════════════════════════════════════
// Group 9 — IRQ delivery
//
// Verifies that the ACIA asserts /IRQ (has_irq() == true) after receiving a
// complete frame when RX interrupts are enabled (CR7 = 1).  This is the path
// the MOS tape-load state machine depends on: it never polls RDRF directly but
// waits for the ACIA IRQ to fire so its handler can advance fsReadProgressState.
// ════════════════════════════════════════════════════════════════════════════

// With CR7=1, receiving a complete byte must assert has_irq().
TEST_F(SulaTest, rx_byte_with_irq_enabled_raises_has_irq) {
  SeqPort seq;
  // 2 mark bits (carrier) then one 8N1 frame for $2A
  seq.bits.push_back(true);
  seq.bits.push_back(true);
  seq.bits.push_back(false);  // start bit
  for (int i = 0; i < 8; ++i) seq.bits.push_back((0x2A >> i) & 1);
  seq.bits.push_back(true);   // stop bit

  sula_->set_cassette_port(&seq);

  // Master reset, then ÷16 8N1 with RX IRQ enabled (CR7=1).
  bus_->set_address(0xFE08); bus_->set_data(0x03); bus_->clr_RW(); acia_->tick(bus_);
  acia_->clear_cts();
  bus_->set_address(0xFE08); bus_->set_data(0xD5); bus_->clr_RW(); acia_->tick(bus_);

  // Motor on, cassette selected.
  bus_->set_address(0xFE10); bus_->set_data(0x80); bus_->clr_RW(); sula_->tick(bus_);

  // Run enough ticks for the full frame to be received.
  const uint32_t ticks_per_bit = 16u * 832u;
  tick_n((2u + 10u + 1u) * ticks_per_bit);  // carrier + frame + 1 bit margin

  EXPECT_TRUE(acia_->has_irq()) << "ACIA must assert /IRQ after receiving byte with CR7=1";
}

// If a byte arrives while CR7=0 (IRQ disabled), enabling CR7=1 afterwards must
// backfill the IRQ for the pending RDRF — otherwise the MOS never gets notified.
TEST_F(SulaTest, enable_rx_irq_after_rdrf_set_backfills_irq) {
  SeqPort seq;
  seq.bits.push_back(true);
  seq.bits.push_back(true);
  seq.bits.push_back(false);  // start bit
  for (int i = 0; i < 8; ++i) seq.bits.push_back((0x2A >> i) & 1);
  seq.bits.push_back(true);   // stop bit

  sula_->set_cassette_port(&seq);

  // Master reset, then ÷16 8N1 with RX IRQ *disabled* (CR7=0).
  bus_->set_address(0xFE08); bus_->set_data(0x03); bus_->clr_RW(); acia_->tick(bus_);
  acia_->clear_cts();
  bus_->set_address(0xFE08); bus_->set_data(0x55); bus_->clr_RW(); acia_->tick(bus_);

  bus_->set_address(0xFE10); bus_->set_data(0x80); bus_->clr_RW(); sula_->tick(bus_);

  const uint32_t ticks_per_bit = 16u * 832u;
  tick_n((2u + 10u + 1u) * ticks_per_bit);

  // Byte should be in RDR but no IRQ yet.
  bus_->set_address(0xFE08); bus_->set_RW(); acia_->tick(bus_);
  ASSERT_TRUE(bus_->get_data() & 0x01) << "RDRF must be set before enabling IRQ";
  ASSERT_FALSE(acia_->has_irq()) << "IRQ must not be asserted while CR7=0";

  // Now enable RX IRQ — must backfill for the pending RDRF.
  bus_->set_address(0xFE08); bus_->set_data(0xD5); bus_->clr_RW(); acia_->tick(bus_);

  EXPECT_TRUE(acia_->has_irq()) << "enabling CR7=1 with RDRF already set must assert /IRQ";
}
