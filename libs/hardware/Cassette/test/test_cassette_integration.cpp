#include <gtest/gtest.h>

#include "cassette_port.h"
#include "uef_tape_stream.h"
#include "2c198_sula.h"
#include "acia_6850.h"
#include "bus.h"

#include <UEF/uef.h>
#include <memory>
#include <sstream>

// ── Minimal UEF builder ──────────────────────────────────────────────────────
// Mirrors helpers in test_uef_tape_stream.cpp; duplicated here so the
// integration test is self-contained.

using RawChunk = std::pair<uint16_t, std::vector<uint8_t>>;

static void put_u16le(std::ostringstream& s, uint16_t v) {
  s.put(char(v & 0xFF)); s.put(char(v >> 8));
}
static void put_u32le(std::ostringstream& s, uint32_t v) {
  s.put(char(v        & 0xFF)); s.put(char((v >>  8) & 0xFF));
  s.put(char((v >> 16) & 0xFF)); s.put(char((v >> 24) & 0xFF));
}
static UefData make_uef(const std::vector<RawChunk>& chunks) {
  std::ostringstream raw;
  raw.write("UEF File!\0", 10);
  raw.put(0x00); raw.put(0x0A);  // minor=0, major=10
  for (auto& [id, pl] : chunks) {
    put_u16le(raw, id);
    put_u32le(raw, uint32_t(pl.size()));
    for (uint8_t b : pl) raw.put(char(b));
  }
  auto str = raw.str();
  std::unique_ptr<std::istream> is = std::make_unique<std::istringstream>(str);
  return UefData::FromStream(is);
}
static RawChunk carrier_chunk(uint16_t n) {
  return {0x0110, {uint8_t(n & 0xFF), uint8_t(n >> 8)}};
}
static RawChunk gap_chunk(uint16_t n) {
  return {0x0112, {uint8_t(n & 0xFF), uint8_t(n >> 8)}};
}
static RawChunk data_chunk(std::vector<uint8_t> bytes) {
  return {0x0100, std::move(bytes)};
}

// ── Fixture ──────────────────────────────────────────────────────────────────

static constexpr uint16_t ACIA_BASE = 0xFE08;
static constexpr uint16_t SULA_BASE = 0xFE10;
static constexpr uint8_t  SR_RDRF   = (1 << 0);
static constexpr uint8_t  SR_DCD    = (1 << 2);

// ticks per tape bit at 1200 baud:
//   sULA rx_clock fires every 13 × 64 = 832 16MHz ticks (cassette fixed divider=64).
//   ACIA clk_divisor=16, so the tape advances once per 16 rx_clocks = 16×832 = 13312 ticks.
static constexpr uint32_t TPB = 16u * 832u;

// ── Carrier chunk sizing ──────────────────────────────────────────────────────
// Real BBC MOS (OS 1.20) writes ≈12,000 carrier cycles (2400 Hz × 5 s) before
// and after each file, and ≈1,440 cycles (0.6 s default) between blocks.
// Tests use minimum viable counts to keep tick_n() fast:
//
//   pre-data  ≥ 1 cycle  sULA initialises current_rx_bit_ to false (SPACE) until
//                        the first tape advance.  At least one MARK cycle must be
//                        clocked in before the start bit so the ACIA begins in the
//                        correct idle (MARK) state.  We use 2 for a clean guard.
//
//   trailing  ≥ 1 cycle  The stop-bit commits mid-bit (8 rx_clocks into the
//                        stop-bit window, due to start-bit phase alignment).  The
//                        NEXT tape advance fires 8 rx_clocks later.  If that
//                        advance returns EOF, raise_dcd() fires and clears RDRF
//                        before the assertion runs.  One trailing carrier cycle
//                        makes that advance return MARK instead, keeping RDRF set.

class CassetteIntegration : public ::testing::Test {
 protected:
  void SetUp() override {
    bus_  = std::make_shared<Bus>();
    acia_ = std::make_shared<Acia>(ACIA_BASE);
    sula_ = std::make_unique<SerialUla>(SULA_BASE);
    sula_->set_acia(acia_);
    sula_->set_cassette_port(&port_);
  }

  void write_acia(uint8_t v) {
    bus_->set_address(ACIA_BASE); bus_->set_data(v); bus_->clr_RW(); acia_->tick(bus_);
  }
  void write_scr(uint8_t v) {
    bus_->set_address(SULA_BASE); bus_->set_data(v); bus_->clr_RW(); sula_->tick(bus_);
  }
  uint8_t read_sr() {
    bus_->set_address(ACIA_BASE);     bus_->set_RW(); acia_->tick(bus_); return bus_->get_data();
  }
  uint8_t read_rdr() {
    bus_->set_address(ACIA_BASE + 1); bus_->set_RW(); acia_->tick(bus_); return bus_->get_data();
  }
  void tick_n(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) sula_->tick_16mhz();
  }

  // Configure ACIA + sULA for 1200-baud cassette RX with RX IRQ enabled.
  void setup_1200_irq() {
    write_acia(0x03);   // master reset
    acia_->clear_cts();
    write_acia(0xD5);   // ÷16, 8N1, RX IRQ enabled
    write_scr(0x80);    // motor on, cassette
  }

  std::shared_ptr<Bus>       bus_;
  std::shared_ptr<Acia>      acia_;
  std::unique_ptr<SerialUla> sula_;
  CassettePort               port_;
};

// ════════════════════════════════════════════════════════════════════════════
// Integration tests — UefTapeStream → CassettePort → SerialUla → Acia
// ════════════════════════════════════════════════════════════════════════════

// Simplest case: a single data byte preceded by carrier arrives in the ACIA RDR.
TEST_F(CassetteIntegration, SingleByte_ReceivedCorrectly) {
  // 2 pre-carrier + 10 data bits + 1 trailing = 13 advances (0..12).
  // Advance 12 (trailing carrier) is the last one consumed by tick_n; it must
  // be MARK so DCD* stays low and RDRF is not cleared before the assertion.
  // Minimum trailing = 1.  Real MOS writes ≈12 000 cycles.
  auto uef = make_uef({carrier_chunk(2), data_chunk({0x42}), carrier_chunk(1)});
  UefTapeStream stream(uef);
  port_.set_stream(&stream);
  setup_1200_irq();

  tick_n((2u + 10u + 1u) * TPB);

  ASSERT_TRUE(read_sr() & SR_RDRF) << "RDRF must be set after receiving frame";
  EXPECT_EQ(read_rdr(), 0x42u);
}

// Multiple bytes in a single data block are all received correctly.
TEST_F(CassetteIntegration, MultipleBytes_AllReceived) {
  // The loop accumulates 13+11+11 = 35 tape advances total.
  // Advances 0–1 = pre-carrier, 2–31 = 30 data bits (3 × 10), 32–34 = trailing.
  // Last advance consumed = 34; must be carrier so DCD* stays low.
  // Minimum trailing = 35 − (2 pre + 30 data) = 3.  Real MOS ≈12 000 cycles.
  auto uef = make_uef({carrier_chunk(2), data_chunk({0x2A, 0xFF, 0x00}), carrier_chunk(3)});
  UefTapeStream stream(uef);
  port_.set_stream(&stream);
  setup_1200_irq();

  const std::vector<uint8_t> expected = {0x2A, 0xFF, 0x00};
  std::vector<uint8_t> received;

  for (size_t i = 0; i < expected.size(); ++i) {
    // Each byte = 10 bits; advance one frame at a time.
    tick_n((i == 0 ? 2u + 10u : 10u) * TPB + TPB);  // +carrier on first byte
    ASSERT_TRUE(read_sr() & SR_RDRF) << "RDRF not set for byte " << i;
    received.push_back(read_rdr());
  }

  EXPECT_EQ(received, expected);
}

// Gap following a byte raises SR2 (DCD latched) and asserts IRQ.
TEST_F(CassetteIntegration, InterBlock_GapRaisesDcdAndIrq) {
  // No trailing carrier before the gap: we WANT DCD* to go high.
  // Real MOS uses ≈1 440 gap cycles; 5 is enough to confirm the latch triggers.
  auto uef = make_uef({carrier_chunk(2), data_chunk({0x42}), gap_chunk(5)});
  UefTapeStream stream(uef);
  port_.set_stream(&stream);
  setup_1200_irq();

  tick_n((2u + 10u + 5u) * TPB);

  EXPECT_TRUE(read_sr() & SR_DCD) << "SR2 must be set during inter-block gap";
  EXPECT_TRUE(acia_->has_irq())   << "IRQ must be asserted after gap";
}

// Full inter-block sequence: latch clears after carrier-returns + read-SR + read-RDR,
// then the second byte arrives and is received correctly.
TEST_F(CassetteIntegration, InterBlock_SecondByteAfterLatchClear) {
  // carrier_chunk(1) after each data block: minimum trailing carrier (see above).
  // Without it the advance that falls inside tick_n's final TPB would be the first
  // gap bit; raise_dcd() would clear RDRF before the assertion observes it.
  // Real MOS writes ≈12 000 carrier cycles before the inter-block gap.
  // gap_chunk(5): 5 cycles sufficient to trigger the DCD latch.
  // Real MOS inter-block gap default ≈1 440 cycles (0.6 s × 2400 Hz).
  auto uef = make_uef({
    carrier_chunk(2), data_chunk({0x42}), carrier_chunk(1),  // block 1 + tail
    gap_chunk(5),                                             // inter-block gap
    carrier_chunk(2), data_chunk({0x55}), carrier_chunk(1),  // block 2 + tail
  });
  UefTapeStream stream(uef);
  port_.set_stream(&stream);
  setup_1200_irq();

  // ── Phase 1: receive 0x42, enter gap ────────────────────────────────────
  tick_n((2u + 10u + 5u) * TPB);
  ASSERT_TRUE(read_sr() & SR_DCD) << "SR2 must be set after gap";

  // ── Phase 2: carrier returns → arms latch clear ─────────────────────────
  tick_n(2u * TPB);
  // read-SR is step 1 of the two-step latch-clear sequence; SR2 still latched.
  ASSERT_TRUE(read_sr() & SR_DCD) << "SR2 must remain latched after carrier returns";

  // read-RDR is step 2: DCD* LOW (carrier present) → SR2 cleared, 0x42 consumed.
  EXPECT_EQ(read_rdr(), 0x42u)    << "first byte must be 0x42";
  EXPECT_FALSE(read_sr() & SR_DCD) << "SR2 must be cleared after latch-clear sequence";

  // ── Phase 3: receive second byte 0x55 ───────────────────────────────────
  // +2 not +1: trailing carrier_chunk(1) after block 1 shifts block 2's data
  // start 1 bit later, so the stop-bit commit tick (advance 30) is 12 TPB away.
  tick_n((10u + 2u) * TPB);

  ASSERT_TRUE(read_sr() & SR_RDRF) << "RDRF must be set for second byte";
  EXPECT_EQ(read_rdr(), 0x55u)     << "second byte must be 0x55";
}
