#include "test_acia.h"

// ── Fixture ──────────────────────────────────────────────────────────────────

void AciaTest::SetUp() {
  bus_  = std::make_shared<Bus>();
  acia_ = std::make_shared<Acia>(BASE);
}

void AciaTest::write_ctl(uint8_t data) {
  bus_->set_address(BASE);
  bus_->set_data(data);
  bus_->clr_RW();
  acia_->tick(bus_);
}

void AciaTest::write_tdr(uint8_t data) {
  bus_->set_address(BASE + 1);
  bus_->set_data(data);
  bus_->clr_RW();
  acia_->tick(bus_);
}

uint8_t AciaTest::read_status() {
  bus_->set_address(BASE);
  bus_->set_RW();
  acia_->tick(bus_);
  return bus_->get_data();
}

uint8_t AciaTest::read_rdr() {
  bus_->set_address(BASE + 1);
  bus_->set_RW();
  acia_->tick(bus_);
  return bus_->get_data();
}

void AciaTest::init(uint8_t ctl) {
  write_ctl(0x03);    // first master reset — releases power-on reset
  acia_->clear_cts(); // cassette selected (CTS active low)
  write_ctl(ctl);
}

void AciaTest::tx_clocks(int n) {
  for (int i = 0; i < n; i++) acia_->tx_clock();
}

std::vector<bool> AciaTest::capture_tx_frame(int word_len, bool has_parity, int stop_bits) {
  // The ÷1 TX state machine advances one state per tx_clock() call.
  // Tick sequence: IDLE (loads TDR→shift reg) → START_BIT → BITS×N → [PARITY] → STOP×M
  std::vector<bool> bits;
  acia_->tx_clock(); // IDLE: loads shift register, no output change
  acia_->tx_clock(); // SEND_START_BIT: outputs 0
  bits.push_back(acia_->tx_pin());
  for (int i = 0; i < word_len; i++) {
    acia_->tx_clock();
    bits.push_back(acia_->tx_pin());
  }
  if (has_parity) {
    acia_->tx_clock();
    bits.push_back(acia_->tx_pin());
  }
  for (int i = 0; i < stop_bits; i++) {
    acia_->tx_clock();
    bits.push_back(acia_->tx_pin());
  }
  return bits;
  // bits layout: [0]=start, [1..word_len]=data, [word_len+1]=parity(opt), last=stop(s)
}

void AciaTest::rx_bit(bool bit, int clocks) {
  acia_->set_rx_data(bit);
  for (int i = 0; i < clocks; i++) acia_->rx_clock();
}

void AciaTest::rx_byte(uint8_t data, int word_len, int clocks_per_bit) {
  rx_bit(false, clocks_per_bit); // start bit
  for (int i = 0; i < word_len; i++)
    rx_bit((data >> i) & 1, clocks_per_bit);
  rx_bit(true, clocks_per_bit); // stop bit
}

// ════════════════════════════════════════════════════════════════════════════
// Power-on reset
// ════════════════════════════════════════════════════════════════════════════

// Before the mandatory first master reset the ACIA ignores all control writes.
TEST_F(AciaTest, control_write_before_master_reset_is_ignored) {
  acia_->clear_cts();
  write_ctl(0x14); // would set /1, 8N1 if accepted — should be ignored
  EXPECT_FALSE(read_status() & SR_TDRE);
}

TEST_F(AciaTest, irq_inactive_on_construction) {
  EXPECT_FALSE(acia_->has_irq());
}

TEST_F(AciaTest, tx_line_is_mark_on_construction) {
  EXPECT_TRUE(acia_->tx_pin());
}

// ════════════════════════════════════════════════════════════════════════════
// Master reset
// ════════════════════════════════════════════════════════════════════════════

// After the first master reset, subsequent control register writes take effect.
TEST_F(AciaTest, first_master_reset_releases_power_on_reset) {
  write_ctl(0x03);    // first master reset
  acia_->clear_cts();
  write_ctl(0x14);    // accepted now: /1, 8N1
  EXPECT_TRUE(read_status() & SR_TDRE);
}

// Second master reset clears error flags, RDRF, and TDRE (but not CTS/DCD bits).
TEST_F(AciaTest, second_master_reset_clears_error_and_data_flags) {
  init();
  acia_->raise_dcd(); // put some noise into status
  write_ctl(0x03);    // second master reset
  auto sr = read_status();
  EXPECT_FALSE(sr & SR_FE);
  EXPECT_FALSE(sr & SR_OVRN);
  EXPECT_FALSE(sr & SR_PE);
  EXPECT_FALSE(sr & SR_RDRF);
}

// After master reset with CTS active, TDRE is set (transmission is possible).
TEST_F(AciaTest, second_master_reset_sets_tdre_when_cts_active) {
  init();
  write_tdr(0xAA); // fill TDR so TDRE clears
  ASSERT_FALSE(read_status() & SR_TDRE);
  write_ctl(0x03); // master reset
  EXPECT_TRUE(read_status() & SR_TDRE);
}

// Master reset does NOT affect the CTS status bit (external input).
TEST_F(AciaTest, master_reset_does_not_clear_cts_status_bit) {
  write_ctl(0x03); // first reset — CTS still inactive (high)
  write_ctl(0x14);
  write_ctl(0x03); // second reset — CTS still inactive
  EXPECT_TRUE(read_status() & SR_CTS);
}

// The BBC Micro MOS sets the ACIA to &56 for cassette idle:
// ÷64, 8 data bits, 1 stop bit, no parity, RTS high, no interrupts.
TEST_F(AciaTest, bbc_normal_config_0x56) {
  // 0x56 = 0101 0110:
  //   CR1:CR0 = 10  → ÷64
  //   CR4:CR3:CR2 = 101 → 8N1
  //   CR6:CR5 = 10  → RTS high, TX int disabled
  //   CR7 = 0       → RX int disabled
  write_ctl(0x03); // master reset
  acia_->clear_cts();
  write_ctl(0x56);
  // With CTS active and TDR empty, TDRE should be set.
  EXPECT_TRUE(read_status() & SR_TDRE);
  // IRQ must be inactive (no interrupts enabled).
  EXPECT_FALSE(acia_->has_irq());
}

// ════════════════════════════════════════════════════════════════════════════
// CTS (Clear To Send)
// ════════════════════════════════════════════════════════════════════════════

TEST_F(AciaTest, clear_cts_sets_tdre_when_tdr_empty) {
  write_ctl(0x03);
  write_ctl(0x14);
  acia_->clear_cts();
  EXPECT_TRUE(read_status() & SR_TDRE);
}

// CTS going active while TDR is full does NOT set TDRE.
TEST_F(AciaTest, clear_cts_does_not_set_tdre_when_tdr_full) {
  init();
  tx_clocks(1);    // shift first byte out of TDR → TDRE set
  write_tdr(0xAA); // fill TDR again
  acia_->raise_cts();
  acia_->clear_cts(); // CTS re-activated with data pending
  EXPECT_FALSE(read_status() & SR_TDRE);
}

TEST_F(AciaTest, raise_cts_clears_tdre) {
  init();
  ASSERT_TRUE(read_status() & SR_TDRE);
  acia_->raise_cts();
  EXPECT_FALSE(read_status() & SR_TDRE);
}

// CTS inactive (high) → SR_CTS=1; CTS active (low) → SR_CTS=0.
TEST_F(AciaTest, cts_status_bit_tracks_cts_pin) {
  write_ctl(0x03);
  write_ctl(0x14);
  EXPECT_TRUE(read_status() & SR_CTS); // CTS inactive
  acia_->clear_cts();
  EXPECT_FALSE(read_status() & SR_CTS); // CTS active
  acia_->raise_cts();
  EXPECT_TRUE(read_status() & SR_CTS); // inactive again
}

// ════════════════════════════════════════════════════════════════════════════
// DCD (Data Carrier Detect)
// ════════════════════════════════════════════════════════════════════════════

// A low-to-high DCD transition (carrier lost) latches the DCD status bit.
TEST_F(AciaTest, raise_dcd_sets_dcd_status_bit) {
  init();
  acia_->raise_dcd();
  EXPECT_TRUE(read_status() & SR_DCD);
}

// DCD going inactive clears RDRF.
TEST_F(AciaTest, raise_dcd_clears_rdrf) {
  init(0x94); // RX ints enabled
  rx_byte(0x42);
  ASSERT_TRUE(read_status() & SR_RDRF);
  acia_->raise_dcd();
  EXPECT_FALSE(read_status() & SR_RDRF);
}

// DCD going inactive raises IRQ when RX interrupts are enabled.
TEST_F(AciaTest, raise_dcd_raises_irq_with_rx_interrupts_enabled) {
  init(0x94);
  acia_->raise_dcd();
  EXPECT_TRUE(acia_->has_irq());
}

// DCD does not raise IRQ when RX interrupts are disabled.
TEST_F(AciaTest, raise_dcd_no_irq_without_rx_interrupts) {
  init(); // no RX ints
  acia_->raise_dcd();
  EXPECT_FALSE(acia_->has_irq());
}

// SR_DCD is latched — stays set even after DCD pin goes active again.
TEST_F(AciaTest, dcd_status_latches_after_dcd_returns_active) {
  init();
  acia_->raise_dcd();
  acia_->clear_dcd(); // carrier returns
  EXPECT_TRUE(read_status() & SR_DCD); // still latched
}

// Clearing the DCD latch requires: DCD* returns low, then 1. read status, 2. read data register.
// The latch clear sequence is armed by clear_dcd() (carrier return), not raise_dcd().
TEST_F(AciaTest, dcd_status_cleared_by_read_status_then_read_data) {
  init();
  acia_->raise_dcd();  // carrier lost → SR[2]=1, sequence NOT yet armed
  acia_->clear_dcd();  // carrier returns → latch sequence armed, SR[2] stays 1
  read_status();       // step 1
  read_rdr();          // step 2; DCD* is now low → SR[2] cleared
  EXPECT_FALSE(read_status() & SR_DCD);
}

// If DCD* remains asserted after read-status + read-data, SR[2] stays high (datasheet §).
TEST_F(AciaTest, dcd_status_remains_high_if_dcd_still_asserted_after_read_sequence) {
  init();
  acia_->raise_dcd();  // carrier still present throughout
  read_status();
  read_rdr();
  EXPECT_TRUE(read_status() & SR_DCD);
}

// Reading only the status register is not enough.
TEST_F(AciaTest, dcd_status_not_cleared_by_status_read_alone) {
  init();
  acia_->raise_dcd();
  read_status();
  EXPECT_TRUE(read_status() & SR_DCD); // still latched after only status read
}

// Reading only the data register (skipping status read) is not enough.
TEST_F(AciaTest, dcd_status_not_cleared_by_data_read_alone) {
  init();
  acia_->raise_dcd();
  read_rdr(); // no prior status read
  EXPECT_TRUE(read_status() & SR_DCD);
}

// Latch clear sequence is not armed until carrier returns (clear_dcd).
// read_status + read_rdr while carrier has never returned must not clear SR2.
TEST_F(AciaTest, dcd_latch_not_armed_until_carrier_returns) {
  init();
  acia_->raise_dcd(); // carrier lost — sequence NOT armed
  // deliberately do NOT call clear_dcd()
  read_status();
  read_rdr();
  EXPECT_TRUE(read_status() & SR_DCD);
}

// raise_dcd() while a character is mid-reception aborts the state machine.
// Subsequent rx_clock() calls (with DCD* high) must not complete the byte.
TEST_F(AciaTest, raise_dcd_aborts_in_progress_reception) {
  init();
  rx_bit(false);         // start bit
  rx_bit(true);          // d0
  rx_bit(false);         // d1
  rx_bit(true);          // d2
  acia_->raise_dcd();    // carrier lost mid-byte
  rx_bit(false);         // d3 — should be blocked
  rx_bit(true);          // d4
  rx_bit(false);         // d5
  rx_bit(true);          // d6
  rx_bit(false);         // d7
  rx_bit(true);          // stop bit
  EXPECT_FALSE(read_status() & SR_RDRF);
}

// rx_clock() does not advance the receiver while DCD* is high.
TEST_F(AciaTest, rx_clock_blocked_while_dcd_high) {
  init();
  acia_->raise_dcd();
  rx_byte(0x42); // send a complete frame — should have no effect
  EXPECT_FALSE(read_status() & SR_RDRF);
}

// Normal reception resumes after DCD* returns low (carrier restored).
TEST_F(AciaTest, rx_resumes_after_dcd_returns_low) {
  init();
  acia_->raise_dcd();
  acia_->clear_dcd();
  rx_byte(0x55);
  EXPECT_TRUE(read_status() & SR_RDRF);
  EXPECT_EQ(read_rdr(), 0x55);
}

// raise_dcd() clears error flags (FE, PE, OVRN) left from a previous receive.
TEST_F(AciaTest, raise_dcd_clears_error_flags) {
  init();
  // Generate a framing error: send all-zero frame including stop bit.
  rx_bit(false);
  for (int i = 0; i < 8; i++) rx_bit(false);
  rx_bit(false); // stop bit should be 1 — framing error
  ASSERT_TRUE(read_status() & SR_FE);
  acia_->raise_dcd();
  EXPECT_FALSE(read_status() & SR_FE);
  EXPECT_FALSE(read_status() & SR_OVRN);
  EXPECT_FALSE(read_status() & SR_PE);
}

// ════════════════════════════════════════════════════════════════════════════
// TX framing
// ════════════════════════════════════════════════════════════════════════════

// TX line is held at mark (1) when idle — no data transmitting.
TEST_F(AciaTest, tx_idle_line_is_mark) {
  init();
  tx_clocks(10); // nothing in TDR; IDLE should keep line high
  EXPECT_TRUE(acia_->tx_pin());
}

// The start bit is always 0 (space).
TEST_F(AciaTest, tx_start_bit_is_zero) {
  init();
  write_tdr(0xFF); // all-ones data
  auto frame = capture_tx_frame(8, false, 1);
  EXPECT_FALSE(frame[0]); // start bit = 0
}

// Data bits are transmitted LSB first (8-bit word).
TEST_F(AciaTest, tx_sends_8bit_data_lsb_first) {
  init(); // /1, 8N1
  // 0xA5 = 1010 0101 → LSB-first: 1,0,1,0,0,1,0,1
  write_tdr(0xA5);
  auto frame = capture_tx_frame(8, false, 1);
  // frame[0]=start, frame[1..8]=data bits d0..d7
  EXPECT_EQ(frame[1], true);  // d0 = 1
  EXPECT_EQ(frame[2], false); // d1 = 0
  EXPECT_EQ(frame[3], true);  // d2 = 1
  EXPECT_EQ(frame[4], false); // d3 = 0
  EXPECT_EQ(frame[5], false); // d4 = 0
  EXPECT_EQ(frame[6], true);  // d5 = 1
  EXPECT_EQ(frame[7], false); // d6 = 0
  EXPECT_EQ(frame[8], true);  // d7 = 1
}

// Data bits are transmitted LSB first (7-bit word, 2 stop bits, even parity).
TEST_F(AciaTest, tx_sends_7bit_data_lsb_first) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x00); // /1, 7E2 (CR4:CR3:CR2=000 → 7 bits, even parity, 2 stop)
  // 0x55 lower 7 bits = 101 0101 → LSB-first: 1,0,1,0,1,0,1
  write_tdr(0x55);
  auto frame = capture_tx_frame(7, true, 2);
  EXPECT_FALSE(frame[0]); // start bit
  EXPECT_EQ(frame[1], true);  // d0
  EXPECT_EQ(frame[2], false); // d1
  EXPECT_EQ(frame[3], true);  // d2
  EXPECT_EQ(frame[4], false); // d3
  EXPECT_EQ(frame[5], true);  // d4
  EXPECT_EQ(frame[6], false); // d5
  EXPECT_EQ(frame[7], true);  // d6
}

// Stop bit(s) are always mark (1).
TEST_F(AciaTest, tx_stop_bit_is_mark) {
  init();
  write_tdr(0x00);
  auto frame = capture_tx_frame(8, false, 1);
  EXPECT_TRUE(frame.back());
}

TEST_F(AciaTest, tx_two_stop_bits_both_mark) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x10); // /1, 8N2 (CR4:CR3:CR2=100)
  write_tdr(0x00);
  auto frame = capture_tx_frame(8, false, 2);
  EXPECT_TRUE(frame[9]);  // stop bit 1
  EXPECT_TRUE(frame[10]); // stop bit 2
}

// Odd parity: total 1s (data + parity bit) must be odd.
TEST_F(AciaTest, tx_odd_parity_makes_total_ones_odd) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x04); // /1, 7O2 (CR4:CR3:CR2=001)
  write_tdr(0x41); // 0x41 lower 7 bits = 100 0001 → two 1s; parity must add a 1 → total 3
  auto frame = capture_tx_frame(7, true, 2);
  int ones = 0;
  for (int i = 1; i <= 7; i++) ones += frame[i] ? 1 : 0;
  ones += frame[8] ? 1 : 0; // parity bit
  EXPECT_EQ(ones % 2, 1);
}

// Even parity: total 1s (data + parity bit) must be even.
TEST_F(AciaTest, tx_even_parity_makes_total_ones_even) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x00); // /1, 7E2 (CR4:CR3:CR2=000)
  write_tdr(0x41); // 0x41 lower 7 bits → two 1s; parity must be 0 → total 2
  auto frame = capture_tx_frame(7, true, 2);
  int ones = 0;
  for (int i = 1; i <= 7; i++) ones += frame[i] ? 1 : 0;
  ones += frame[8] ? 1 : 0;
  EXPECT_EQ(ones % 2, 0);
}

// TDRE goes high once the shift register loads from TDR (CPU can write next byte).
TEST_F(AciaTest, tx_tdre_set_after_tdr_loads_into_shift_register) {
  init();
  write_tdr(0xAA);
  ASSERT_FALSE(read_status() & SR_TDRE);
  tx_clocks(1); // IDLE tick: loads shift register → TDR emptied → TDRE set
  EXPECT_TRUE(read_status() & SR_TDRE);
}

// With TX interrupts enabled, IRQ fires when TDRE is set.
TEST_F(AciaTest, tx_irq_raised_when_tdre_set_and_tx_int_enabled) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x34); // /1, 8N1, TX int enabled (CR6:CR5=01)
  // CTS active + TDR empty → TDRE is set → IRQ fires immediately upon enabling TX int
  EXPECT_TRUE(acia_->has_irq());
}

// Writing to TDR clears IRQ.
TEST_F(AciaTest, tx_write_tdr_clears_irq) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x34);
  ASSERT_TRUE(acia_->has_irq());
  write_tdr(0xAA);
  EXPECT_FALSE(acia_->has_irq());
}

// Double-buffering: a second byte written to TDR while the first is transmitting
// will automatically load into the shift register when the first is done.
TEST_F(AciaTest, tx_double_buffer_second_byte_transmits_automatically) {
  init(); // /1, 8N1
  write_tdr(0x41); // first byte
  tx_clocks(1);    // IDLE: shifts into SR, TDRE set
  ASSERT_TRUE(read_status() & SR_TDRE);
  write_tdr(0x42); // second byte — TDR full, TDRE cleared
  ASSERT_FALSE(read_status() & SR_TDRE);

  // Transmit first byte: 1 start + 8 data + 1 stop = 10 more ticks
  tx_clocks(10);

  // Now at IDLE again — next tick should auto-load second byte, setting TDRE
  tx_clocks(1);
  EXPECT_TRUE(read_status() & SR_TDRE);
}

// CR6=1, CR5=1 → transmit break: TX line held continuously low (space).
TEST_F(AciaTest, tx_break_level_holds_line_low) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x74); // /1, 8N1, break (CR6:CR5=11)
  // In break mode the TX output should be continuously space (0).
  // Fire several clocks and check the output never goes high.
  for (int i = 0; i < 20; i++) {
    acia_->tx_clock();
    EXPECT_FALSE(acia_->tx_pin()) << "TX should be held low (break) at tick " << i;
  }
}

// CTS inactive (high) completely inhibits TX clock processing.
TEST_F(AciaTest, tx_cts_inactive_inhibits_transmission) {
  init();
  write_tdr(0xAA);
  acia_->raise_cts(); // CTS inactive
  tx_clocks(20);
  // Shift register never loaded — TDRE must remain low.
  EXPECT_FALSE(read_status() & SR_TDRE);
}

// ════════════════════════════════════════════════════════════════════════════
// RX framing  (÷1 mode — one rx_clock() call per bit period)
// ════════════════════════════════════════════════════════════════════════════

// Clocking with line held at mark (idle) never sets RDRF.
TEST_F(AciaTest, rx_idle_line_does_not_set_rdrf) {
  init();
  for (int i = 0; i < 100; i++) {
    acia_->set_rx_data(true);
    acia_->rx_clock();
  }
  EXPECT_FALSE(read_status() & SR_RDRF);
}

// A complete start + 8 data + stop frame sets RDRF.
TEST_F(AciaTest, rx_complete_frame_sets_rdrf) {
  init();
  rx_byte(0x42);
  EXPECT_TRUE(read_status() & SR_RDRF);
}

// The correct byte value is stored in the RDR.
TEST_F(AciaTest, rx_received_byte_value_correct) {
  init();
  rx_byte(0xA5);
  ASSERT_TRUE(read_status() & SR_RDRF);
  EXPECT_EQ(read_rdr(), 0xA5);
}

TEST_F(AciaTest, rx_all_zeros_byte) {
  init();
  rx_byte(0x00);
  ASSERT_TRUE(read_status() & SR_RDRF);
  EXPECT_EQ(read_rdr(), 0x00);
}

TEST_F(AciaTest, rx_all_ones_byte) {
  init();
  rx_byte(0xFF);
  ASSERT_TRUE(read_status() & SR_RDRF);
  EXPECT_EQ(read_rdr(), 0xFF);
}

// For 7-bit + parity mode, bit 7 of the RDR is cleared (datasheet: "data alone is
// transferred to the MPU").
TEST_F(AciaTest, rx_7bit_word_bit7_cleared_in_rdr) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x00); // /1, 7E2
  // Send 0x7F (7 data bits, all ones). Even parity of 7 ones = 1 (to make total 8 = even).
  // Frame: start(0) d0..d6(all 1) parity(1) stop1(1) stop2(1)
  rx_bit(false);
  for (int i = 0; i < 7; i++) rx_bit(true);  // 7 data bits = 0x7F
  rx_bit(true);  // even parity: 7 ones → parity bit = 1
  rx_bit(true);  // stop bit 1
  rx_bit(true);  // stop bit 2
  ASSERT_TRUE(read_status() & SR_RDRF);
  EXPECT_EQ(read_rdr() & 0x80, 0); // bit 7 must be 0
}

// IRQ fires when RDRF is set and RX interrupts are enabled.
TEST_F(AciaTest, rx_irq_raised_when_rdrf_and_rx_int_enabled) {
  init(0x94); // RX ints enabled
  rx_byte(0x42);
  EXPECT_TRUE(acia_->has_irq());
}

TEST_F(AciaTest, rx_no_irq_when_rx_int_disabled) {
  init(); // no RX ints
  rx_byte(0x42);
  EXPECT_FALSE(acia_->has_irq());
}

// CPU reading RDR clears RDRF.
TEST_F(AciaTest, rx_rdrf_cleared_after_rdr_read) {
  init();
  rx_byte(0x42);
  ASSERT_TRUE(read_status() & SR_RDRF);
  read_rdr();
  EXPECT_FALSE(read_status() & SR_RDRF);
}

// Reading RDR clears IRQ when RDRF triggered it.
TEST_F(AciaTest, rx_irq_cleared_by_reading_rdr) {
  init(0x94);
  rx_byte(0x42);
  ASSERT_TRUE(acia_->has_irq());
  read_rdr();
  EXPECT_FALSE(acia_->has_irq());
}

// A second byte arriving before the first is read sets the overrun flag.
// Per datasheet: overrun begins at the midpoint of the last bit of the
// second character. It does NOT appear in status until the valid (first)
// character has been read.
TEST_F(AciaTest, rx_overrun_not_visible_until_first_byte_read) {
  init();
  rx_byte(0x41); // first byte — not read by CPU
  rx_byte(0x42); // second byte — overrun pending
  EXPECT_FALSE(read_status() & SR_OVRN); // not yet visible
  read_rdr();                            // read first byte
  EXPECT_TRUE(read_status() & SR_OVRN);  // now visible
}

// During overrun, RDRF remains set (first valid byte still available).
TEST_F(AciaTest, rx_rdrf_remains_set_during_overrun) {
  init();
  rx_byte(0x41);
  rx_byte(0x42);
  EXPECT_TRUE(read_status() & SR_RDRF);
}

// Missing stop bit (line held low) sets the Framing Error flag.
TEST_F(AciaTest, rx_framing_error_on_missing_stop_bit) {
  init();
  rx_bit(false);             // start bit
  for (int i = 0; i < 8; i++) rx_bit(false); // data bits (all zero)
  rx_bit(false);             // stop bit — should be 1, sending 0 = framing error
  EXPECT_TRUE(read_status() & SR_FE);
}

// Framing error is cleared after master reset.
TEST_F(AciaTest, rx_framing_error_cleared_by_master_reset) {
  init();
  rx_bit(false);
  for (int i = 0; i < 8; i++) rx_bit(false);
  rx_bit(false); // framing error
  ASSERT_TRUE(read_status() & SR_FE);
  write_ctl(0x03); // master reset
  EXPECT_FALSE(read_status() & SR_FE);
}

// Parity error: when received parity does not match the configured mode.
TEST_F(AciaTest, rx_parity_error_on_wrong_parity_bit) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x00); // /1, 7E2 (even parity expected)
  // Send 0x41 (lower 7 bits: two 1s). Correct even parity = 0.
  // Send deliberately wrong parity bit (1) to trigger PE.
  rx_bit(false);      // start bit
  for (int i = 0; i < 7; i++) rx_bit((0x41 >> i) & 1); // 7 data bits
  rx_bit(true);       // parity = 1 (wrong: should be 0 for even parity with two 1s)
  rx_bit(true);       // stop bit
  rx_bit(true);       // stop bit 2
  EXPECT_TRUE(read_status() & SR_PE);
}

// ════════════════════════════════════════════════════════════════════════════
// False start bit deletion (÷16 mode)
// ════════════════════════════════════════════════════════════════════════════

// In ÷16 mode the ACIA requires a full half-bit period (8 consecutive low
// samples) before accepting the start bit. A shorter glitch is silently
// ignored (false start bit deletion, datasheet section "RECEIVE").
TEST_F(AciaTest, rx_false_start_bit_glitch_rejected_in_div16_mode) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x15); // /16, 8N1 (CR1:CR0=01, CR4:CR3:CR2=101)

  // A glitch: fewer than 8 low clocks, then line returns high.
  rx_bit(false, 7); // 7 clocks low — insufficient for start bit
  rx_bit(true,  1); // glitch over

  EXPECT_FALSE(read_status() & SR_RDRF);
}

// A proper start bit (8+ low samples in ÷16 mode) IS accepted.
TEST_F(AciaTest, rx_valid_start_bit_accepted_in_div16_mode) {
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0x15); // /16, 8N1

  rx_byte(0x42, 8, 16); // 8 data bits, 16 rx_clocks per bit
  EXPECT_TRUE(read_status() & SR_RDRF);
}

// ════════════════════════════════════════════════════════════════════════════
// IRQ re-evaluation after clearing
// ════════════════════════════════════════════════════════════════════════════

// The 6850 IRQ pin is level-sensitive: it stays asserted as long as any enabled
// source remains active.  Reading RDR clears the RDRF source, but if TDRE + TX
// interrupt enable is still asserted the IRQ output must stay low.
TEST_F(AciaTest, irq_remains_after_rdr_read_when_tdre_and_tx_int_active) {
  // 0xB4 = /1, 8N1, TX int enabled (CR6:CR5=01), RX int enabled (CR7=1)
  write_ctl(0x03);
  acia_->clear_cts();
  write_ctl(0xB4);
  ASSERT_TRUE(acia_->has_irq());  // TDRE + TX int → IRQ
  write_tdr(0xAA);                // fill TDR: TDRE→0, IRQ cleared
  ASSERT_FALSE(acia_->has_irq());

  rx_byte(0x42);                  // receive byte: RDRF=1, RX int → IRQ
  ASSERT_TRUE(acia_->has_irq());

  tx_clocks(1);                   // shift TDR→SR: TDRE→1, TX int re-fires
  ASSERT_TRUE(read_status() & SR_TDRE);

  // Both sources active. Read RDR clears RDRF, but TDRE+TX int still pending.
  read_rdr();
  EXPECT_TRUE(acia_->has_irq());
}

// ════════════════════════════════════════════════════════════════════════════
// DCD status after master reset
// ════════════════════════════════════════════════════════════════════════════

// Datasheet: "If the DCD input remains high after ... master reset has occurred,
// the interrupt is cleared, the DCD status bit remains high and will follow the input."
TEST_F(AciaTest, dcd_status_remains_set_after_master_reset_if_dcd_still_high) {
  init(0x94);              // RX ints enabled
  acia_->raise_dcd();      // carrier lost — status latches, IRQ fires
  ASSERT_TRUE(read_status() & SR_DCD);
  ASSERT_TRUE(acia_->has_irq());
  write_ctl(0x03);         // master reset — DCD input still physically high
  EXPECT_TRUE(read_status() & SR_DCD);   // status bit follows input: remains set
  EXPECT_FALSE(acia_->has_irq());        // but interrupt is cleared by the reset
}

// ════════════════════════════════════════════════════════════════════════════
// FE/OVRN interaction
// ════════════════════════════════════════════════════════════════════════════

// FE is "present throughout the time that the associated character is available."
// When the errored character is consumed by a read, FE clears — even if that same
// read also makes a pending overrun visible.
TEST_F(AciaTest, fe_cleared_and_ovrn_visible_when_errored_char_read_during_overrun) {
  init();
  // Receive a frame with a framing error (bad stop bit).
  rx_bit(false);
  for (int i = 0; i < 8; i++) rx_bit(false);
  rx_bit(false); // stop bit should be 1 — framing error
  ASSERT_TRUE(read_status() & SR_FE);
  ASSERT_TRUE(read_status() & SR_RDRF);
  // Receive a second frame before the first is read → overrun pending.
  rx_byte(0x55);
  EXPECT_TRUE(read_status() & SR_FE);    // FE persists while char 1 is in RDR
  EXPECT_FALSE(read_status() & SR_OVRN); // OVRN not yet visible
  // Read the first (errored) character.
  read_rdr();
  EXPECT_FALSE(read_status() & SR_FE);   // FE cleared: character consumed
  EXPECT_TRUE(read_status() & SR_OVRN);  // OVRN now visible
}

// ════════════════════════════════════════════════════════════════════════════
// IRQ clearing
// ════════════════════════════════════════════════════════════════════════════

TEST_F(AciaTest, irq_cleared_by_master_reset) {
  init(0x94);
  acia_->raise_dcd();
  ASSERT_TRUE(acia_->has_irq());
  write_ctl(0x03);
  EXPECT_FALSE(acia_->has_irq());
}

// IRQ bit in status register mirrors has_irq().
TEST_F(AciaTest, irq_status_bit_mirrors_has_irq) {
  init(0x94);
  EXPECT_FALSE(read_status() & SR_IRQ); // no IRQ
  EXPECT_FALSE(acia_->has_irq());
  acia_->raise_dcd();
  EXPECT_TRUE(read_status() & SR_IRQ);  // IRQ set
  EXPECT_TRUE(acia_->has_irq());
}
