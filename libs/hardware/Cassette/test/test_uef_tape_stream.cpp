#include <gtest/gtest.h>
#include "uef_tape_stream.h"

#include <sstream>
#include <cstring>

// ---------------------------------------------------------------------------
// Binary UEF construction helpers
// ---------------------------------------------------------------------------

static void write_u16le(std::ostringstream& s, uint16_t v) {
  s.put(v & 0xFF);
  s.put((v >> 8) & 0xFF);
}

static void write_u32le(std::ostringstream& s, uint32_t v) {
  s.put( v        & 0xFF);
  s.put((v >>  8) & 0xFF);
  s.put((v >> 16) & 0xFF);
  s.put((v >> 24) & 0xFF);
}

// Append a UEF chunk: id, payload bytes
static void write_chunk(std::ostringstream& s, uint16_t id,
                         const std::vector<uint8_t>& payload) {
  write_u16le(s, id);
  write_u32le(s, static_cast<uint32_t>(payload.size()));
  for (uint8_t b : payload) s.put(static_cast<char>(b));
}

// Build a complete UEF stream from a list of raw chunks
static UefData make_uef(const std::vector<std::pair<uint16_t, std::vector<uint8_t>>>& chunks) {
  std::ostringstream raw;
  // UEF header: magic + minor(0) + major(10)
  raw.write("UEF File!\0", 10);
  raw.put(0x00);  // minor
  raw.put(0x0A);  // major
  for (auto& [id, payload] : chunks)
    write_chunk(raw, id, payload);

  auto str = raw.str();
  std::unique_ptr<std::istream> is = std::make_unique<std::istringstream>(str);
  return UefData::FromStream(is);
}

// Convenience: carrier tone chunk (0x0110), n cycles
static std::pair<uint16_t, std::vector<uint8_t>> carrier_chunk(uint16_t cycles) {
  return {0x0110, {static_cast<uint8_t>(cycles & 0xFF),
                   static_cast<uint8_t>(cycles >> 8)}};
}

// Convenience: integer gap chunk (0x0112), n units
static std::pair<uint16_t, std::vector<uint8_t>> gap_chunk(uint16_t duration) {
  return {0x0112, {static_cast<uint8_t>(duration & 0xFF),
                   static_cast<uint8_t>(duration >> 8)}};
}

// Convenience: data block chunk (0x0100)
static std::pair<uint16_t, std::vector<uint8_t>> data_chunk(std::vector<uint8_t> bytes) {
  return {0x0100, std::move(bytes)};
}

// Drain all bits from stream and return them
static std::vector<bool> drain(UefTapeStream& s) {
  std::vector<bool> bits;
  while (!s.end_of_tape())
    bits.push_back(s.next_bit());
  return bits;
}

// ===========================================================================
// Group 1 — Empty UEF
// ===========================================================================

TEST(UefTapeStream, EmptyUef_EndOfTape) {
  auto uef = make_uef({});
  UefTapeStream s(uef);
  EXPECT_TRUE(s.end_of_tape());
}

TEST(UefTapeStream, EmptyUef_AtCarrierFalse) {
  auto uef = make_uef({});
  UefTapeStream s(uef);
  EXPECT_FALSE(s.at_carrier());
}

TEST(UefTapeStream, EmptyUef_NextBitReturnsMark) {
  auto uef = make_uef({});
  UefTapeStream s(uef);
  EXPECT_TRUE(s.next_bit());
}

// ===========================================================================
// Group 2 — Carrier tone (0x0110)
// ===========================================================================

TEST(UefTapeStream, CarrierTone_AtCarrierTrue) {
  auto uef = make_uef({carrier_chunk(10)});
  UefTapeStream s(uef);
  EXPECT_TRUE(s.at_carrier());
}

TEST(UefTapeStream, CarrierTone_EmitsMarkBits) {
  auto uef = make_uef({carrier_chunk(5)});
  UefTapeStream s(uef);
  for (int i = 0; i < 5; ++i)
    EXPECT_TRUE(s.next_bit()) << "bit " << i;
}

TEST(UefTapeStream, CarrierTone_ExactCycleCount) {
  auto uef = make_uef({carrier_chunk(8)});
  UefTapeStream s(uef);
  for (int i = 0; i < 8; ++i) s.next_bit();
  EXPECT_TRUE(s.end_of_tape());
}

TEST(UefTapeStream, CarrierTone_AtCarrierFalseAfterExhausted) {
  auto uef = make_uef({carrier_chunk(3)});
  UefTapeStream s(uef);
  for (int i = 0; i < 3; ++i) s.next_bit();
  EXPECT_FALSE(s.at_carrier());
}

// ===========================================================================
// Group 3 — Integer gap (0x0112)
// ===========================================================================

TEST(UefTapeStream, IntegerGap_AtCarrierFalse) {
  auto uef = make_uef({gap_chunk(10)});
  UefTapeStream s(uef);
  s.next_bit();
  EXPECT_FALSE(s.at_carrier());
}

TEST(UefTapeStream, IntegerGap_EmitsMarkBits) {
  auto uef = make_uef({gap_chunk(4)});
  UefTapeStream s(uef);
  for (int i = 0; i < 4; ++i)
    EXPECT_TRUE(s.next_bit()) << "gap bit " << i;
}

TEST(UefTapeStream, IntegerGap_ExactDuration) {
  auto uef = make_uef({gap_chunk(6)});
  UefTapeStream s(uef);
  for (int i = 0; i < 6; ++i) s.next_bit();
  EXPECT_TRUE(s.end_of_tape());
}

// ===========================================================================
// Group 4 — Data block (0x0100), 8N1 framing
// ===========================================================================

TEST(UefTapeStream, DataBlock_AtCarrierTrue) {
  // On real BBC hardware the carrier-detect circuit stays locked during FSK
  // data — DCD* only releases on silence (GAP). AUG §14.2.5 confirms the
  // circuit detects gaps between blocks, not the carrier→data transition.
  auto uef = make_uef({data_chunk({0x00})});
  UefTapeStream s(uef);
  s.next_bit();
  EXPECT_TRUE(s.at_carrier());
}

TEST(UefTapeStream, DataBlock_StartBitIsZero) {
  auto uef = make_uef({data_chunk({0xFF})});
  UefTapeStream s(uef);
  EXPECT_FALSE(s.next_bit());  // start
}

TEST(UefTapeStream, DataBlock_StopBitIsOne) {
  auto uef = make_uef({data_chunk({0x00})});
  UefTapeStream s(uef);
  for (int i = 0; i < 9; ++i) s.next_bit();  // start + 8 data
  EXPECT_TRUE(s.next_bit());   // stop
}

TEST(UefTapeStream, DataBlock_0x01_LsbFirst) {
  auto uef = make_uef({data_chunk({0x01})});
  UefTapeStream s(uef);
  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_TRUE(s.next_bit());   // d0 = 1
  for (int i = 1; i < 8; ++i)
    EXPECT_FALSE(s.next_bit()) << "d" << i;
  EXPECT_TRUE(s.next_bit());   // stop
}

TEST(UefTapeStream, DataBlock_0xAA_Alternating) {
  // 0xAA = 1010_1010 → d0=0,d1=1,d2=0,d3=1,d4=0,d5=1,d6=0,d7=1
  auto uef = make_uef({data_chunk({0xAA})});
  UefTapeStream s(uef);
  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_FALSE(s.next_bit());  // d0=0
  EXPECT_TRUE (s.next_bit());  // d1=1
  EXPECT_FALSE(s.next_bit());  // d2=0
  EXPECT_TRUE (s.next_bit());  // d3=1
  EXPECT_FALSE(s.next_bit());  // d4=0
  EXPECT_TRUE (s.next_bit());  // d5=1
  EXPECT_FALSE(s.next_bit());  // d6=0
  EXPECT_TRUE (s.next_bit());  // d7=1
  EXPECT_TRUE (s.next_bit());  // stop
}

TEST(UefTapeStream, DataBlock_MultiByte_ConsecutiveFrames) {
  // Two bytes: 0x01, 0x02 — should be 20 bits total
  auto uef = make_uef({data_chunk({0x01, 0x02})});
  UefTapeStream s(uef);
  auto bits = drain(s);
  EXPECT_EQ(20u, bits.size());
  EXPECT_TRUE(s.end_of_tape());
}

TEST(UefTapeStream, DataBlock_EndOfTapeAfterLastBit) {
  auto uef = make_uef({data_chunk({0xFF})});
  UefTapeStream s(uef);
  for (int i = 0; i < 10; ++i) s.next_bit();
  EXPECT_TRUE(s.end_of_tape());
}

// ===========================================================================
// Group 5 — Carrier → data → carrier sequence
// ===========================================================================

TEST(UefTapeStream, Sequence_AtCarrier_TransitionsCorrectly) {
  // carrier(3) → data(1 byte) → carrier(3)
  auto uef = make_uef({carrier_chunk(3), data_chunk({0x00}), carrier_chunk(3)});
  UefTapeStream s(uef);

  // Initial state before first bit: carrier
  EXPECT_TRUE(s.at_carrier());

  // at_carrier() is checked AFTER next_bit() — it reflects the bit just returned.
  // First 3 bits: carrier
  for (int i = 0; i < 3; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "after carrier bit " << i;
  }
  // Next 10 bits: data — DCD stays asserted (carrier-detect remains locked)
  for (int i = 0; i < 10; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "after data bit " << i;
  }
  // Final 3 bits: carrier again.  After the very last bit the tape ends,
  // so only check at_carrier() after the first two post-carrier bits.
  for (int i = 0; i < 2; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "after post-carrier bit " << i;
  }
  s.next_bit();  // last bit; tape now exhausted
  EXPECT_TRUE(s.end_of_tape());
  EXPECT_FALSE(s.at_carrier());  // no carrier after tape ends
}

// at_carrier() after next_bit() should reflect the bit just emitted
TEST(UefTapeStream, Sequence_AtCarrierMatchesLastBit) {
  // carrier(2) → data(1 byte)
  auto uef = make_uef({carrier_chunk(2), data_chunk({0x00})});
  UefTapeStream s(uef);

  s.next_bit();              // carrier bit 0
  EXPECT_TRUE(s.at_carrier());
  s.next_bit();              // carrier bit 1 (last carrier bit)
  EXPECT_TRUE(s.at_carrier());
  s.next_bit();              // first data bit (start) — DCD stays asserted
  EXPECT_TRUE(s.at_carrier());
}

// ===========================================================================
// Group 6 — Explicit tape data block (0x0102)
// ===========================================================================

// 0x0102: first byte = ignored bit count, then data bytes, LSB first
TEST(UefTapeStream, ExplicitBlock_EmitsBitsLsbFirst) {
  // 8 bits from 0x01: d0=1, d1-d7=0
  // ignored_bits=0, data byte=0x01
  std::vector<uint8_t> payload = {0x00, 0x01};  // ignored=0, byte=0x01
  auto uef = make_uef({{0x0102, payload}});
  UefTapeStream s(uef);

  EXPECT_TRUE (s.next_bit());  // bit 0 = 1 (LSB of 0x01)
  for (int i = 1; i < 8; ++i)
    EXPECT_FALSE(s.next_bit()) << "bit " << i;
  EXPECT_TRUE(s.end_of_tape());
}

TEST(UefTapeStream, ExplicitBlock_IgnoredBitsExcluded) {
  // 1 byte payload, 4 ignored bits → 4 bits emitted from 0xF0 = 0000 (lower 4)
  std::vector<uint8_t> payload = {0x04, 0xF0};  // 4 ignored bits, byte=0xF0
  auto uef = make_uef({{0x0102, payload}});
  UefTapeStream s(uef);

  // 8 total - 4 ignored = 4 bits: 0xF0 bits 0-3 = 0,0,0,0
  for (int i = 0; i < 4; ++i)
    EXPECT_FALSE(s.next_bit()) << "bit " << i;
  EXPECT_TRUE(s.end_of_tape());
}

// ===========================================================================
// Group 7 — Carrier tone with dummy byte (0x0111)
// ===========================================================================

TEST(UefTapeStream, CarrierWithDummyByte_Structure) {
  // pre=3 cycles, 0xAA (8N1 = 10 bits), post=2 cycles
  std::vector<uint8_t> payload = {0x03, 0x00, 0x02, 0x00};
  auto uef = make_uef({{0x0111, payload}});
  UefTapeStream s(uef);

  // at_carrier() is checked AFTER next_bit() — it reflects the bit just returned.
  // Initial state: pre-carrier segment
  EXPECT_TRUE(s.at_carrier());

  // 3 carrier bits
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(s.next_bit());
    EXPECT_TRUE(s.at_carrier());
  }
  // 10 data bits for 0xAA: start(0) 0,1,0,1,0,1,0,1 stop(1)
  // DCD stays asserted through the data segment (carrier-detect remains locked).
  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_TRUE(s.at_carrier());
  EXPECT_FALSE(s.next_bit());  // d0=0
  EXPECT_TRUE (s.next_bit());  // d1=1
  EXPECT_FALSE(s.next_bit());  // d2=0
  EXPECT_TRUE (s.next_bit());  // d3=1
  EXPECT_FALSE(s.next_bit());  // d4=0
  EXPECT_TRUE (s.next_bit());  // d5=1
  EXPECT_FALSE(s.next_bit());  // d6=0
  EXPECT_TRUE (s.next_bit());  // d7=1
  EXPECT_TRUE (s.next_bit());  // stop
  EXPECT_TRUE(s.at_carrier());
  // 2 post-carrier bits (tape ends after last one)
  EXPECT_TRUE(s.next_bit());   // post-carrier bit 0
  EXPECT_TRUE(s.at_carrier());
  EXPECT_TRUE(s.next_bit());   // post-carrier bit 1 — tape ends
  EXPECT_TRUE(s.end_of_tape());
  EXPECT_FALSE(s.at_carrier());
}

// ===========================================================================
// Group 8 — Integration: synthetic multi-chunk tape
//
// Models a typical BBC Micro tape structure:
//   carrier leader (5) → data byte 0x42 (10 bits) → gap (3) →
//   carrier (4) → data byte 0x0F (10 bits) → carrier trailer (2)
//
// Total bits: 5 + 10 + 3 + 4 + 10 + 2 = 34
//
// DCD*/at_carrier() behaviour (AUG §14.2.5):
//   CARRIER → true   DATA → true   GAP → false
// ===========================================================================

static UefData make_realistic_tape() {
  return make_uef({
    carrier_chunk(5),
    data_chunk({0x42}),
    gap_chunk(3),
    carrier_chunk(4),
    data_chunk({0x0F}),
    carrier_chunk(2),
  });
}

TEST(UefTapeStream, Integration_TotalBitCount) {
  auto uef = make_realistic_tape();
  UefTapeStream s(uef);
  auto bits = drain(s);
  EXPECT_EQ(34u, bits.size());
}

TEST(UefTapeStream, Integration_StartsWithCarrier) {
  auto uef = make_realistic_tape();
  UefTapeStream s(uef);
  EXPECT_TRUE(s.at_carrier());
}

TEST(UefTapeStream, Integration_CarrierDataTransitions) {
  auto uef = make_realistic_tape();
  UefTapeStream s(uef);

  // 5 carrier bits
  for (int i = 0; i < 5; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "carrier leader bit " << i;
  }
  // 10 data bits (0x42 = 0100_0010, start=0, d0-d7 lsb-first, stop=1)
  // DCD stays asserted — carrier-detect remains locked during FSK data.
  for (int i = 0; i < 10; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "first data byte bit " << i;
  }
  // 3 gap bits
  for (int i = 0; i < 3; ++i) {
    s.next_bit();
    EXPECT_FALSE(s.at_carrier()) << "gap bit " << i;
  }
  // 4 carrier bits
  for (int i = 0; i < 4; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "mid carrier bit " << i;
  }
  // 10 data bits (0x0F) — DCD stays asserted
  for (int i = 0; i < 10; ++i) {
    s.next_bit();
    EXPECT_TRUE(s.at_carrier()) << "second data byte bit " << i;
  }
  // 2 carrier trailer bits (last bit ends tape)
  s.next_bit();
  EXPECT_TRUE(s.at_carrier());
  s.next_bit();
  EXPECT_TRUE(s.end_of_tape());
}

TEST(UefTapeStream, Integration_FirstDataByte_0x42) {
  // 0x42 = 0100_0010: start=0, d0=0,d1=1,d2=0,d3=0,d4=0,d5=0,d6=1,d7=0, stop=1
  auto uef = make_realistic_tape();
  UefTapeStream s(uef);
  // skip carrier leader
  for (int i = 0; i < 5; ++i) s.next_bit();

  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_FALSE(s.next_bit());  // d0=0
  EXPECT_TRUE (s.next_bit());  // d1=1
  EXPECT_FALSE(s.next_bit());  // d2=0
  EXPECT_FALSE(s.next_bit());  // d3=0
  EXPECT_FALSE(s.next_bit());  // d4=0
  EXPECT_FALSE(s.next_bit());  // d5=0
  EXPECT_TRUE (s.next_bit());  // d6=1
  EXPECT_FALSE(s.next_bit());  // d7=0
  EXPECT_TRUE (s.next_bit());  // stop
}
