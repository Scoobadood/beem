#include <gtest/gtest.h>
#include "raw_bytes_tape_stream.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Drain all 10 bits for one 8N1 frame and return them as a 10-bit value:
//   bit 0  = first bit returned (start)
//   bit 9  = last bit returned  (stop)
static uint16_t drain_frame(RawBytesTapeStream& s) {
  uint16_t frame = 0;
  for (int i = 0; i < 10; ++i)
    if (s.next_bit()) frame |= (1u << i);
  return frame;
}

// ===========================================================================
// end_of_tape / at_carrier
// ===========================================================================

TEST(RawBytesTapeStream, EmptyStream_EndOfTape) {
  RawBytesTapeStream s({});
  EXPECT_TRUE(s.end_of_tape());
}

TEST(RawBytesTapeStream, NonEmptyStream_NotEndOfTape) {
  RawBytesTapeStream s({0xAA});
  EXPECT_FALSE(s.end_of_tape());
}

TEST(RawBytesTapeStream, AtCarrier_TrueWhileDataRemains) {
  RawBytesTapeStream s({0x00});
  EXPECT_TRUE(s.at_carrier());
}

TEST(RawBytesTapeStream, AtCarrier_FalseWhenExhausted) {
  RawBytesTapeStream s({0x00});
  drain_frame(s);
  EXPECT_TRUE(s.end_of_tape());
  EXPECT_FALSE(s.at_carrier());
}

TEST(RawBytesTapeStream, NextBit_ReturnsMark_AfterEndOfTape) {
  RawBytesTapeStream s({0x00});
  drain_frame(s);
  EXPECT_TRUE(s.next_bit());
  EXPECT_TRUE(s.next_bit());
}

// ===========================================================================
// 8N1 framing — start bit, data bits (LSB first), stop bit
// ===========================================================================

TEST(RawBytesTapeStream, StartBitIsZero) {
  RawBytesTapeStream s({0xFF});
  EXPECT_FALSE(s.next_bit());  // start bit = 0
}

TEST(RawBytesTapeStream, StopBitIsOne) {
  RawBytesTapeStream s({0x00});
  // consume start + 8 data bits
  for (int i = 0; i < 9; ++i) s.next_bit();
  EXPECT_TRUE(s.next_bit());   // stop bit = 1
}

TEST(RawBytesTapeStream, DataBits_0x01_LsbFirst) {
  // 0x01 → bit0=1, bits1-7=0
  RawBytesTapeStream s({0x01});
  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_TRUE(s.next_bit());   // bit 0 = 1
  for (int i = 1; i < 8; ++i)
    EXPECT_FALSE(s.next_bit()) << "bit " << i << " should be 0";
  EXPECT_TRUE(s.next_bit());   // stop
}

TEST(RawBytesTapeStream, DataBits_0x80_LsbFirst) {
  // 0x80 → bits0-6=0, bit7=1
  RawBytesTapeStream s({0x80});
  EXPECT_FALSE(s.next_bit());  // start
  for (int i = 0; i < 7; ++i)
    EXPECT_FALSE(s.next_bit()) << "bit " << i << " should be 0";
  EXPECT_TRUE(s.next_bit());   // bit 7 = 1
  EXPECT_TRUE(s.next_bit());   // stop
}

TEST(RawBytesTapeStream, DataBits_0xAA_Alternating) {
  // 0xAA = 1010_1010 → bits: 0,1,0,1,0,1,0,1
  RawBytesTapeStream s({0xAA});
  uint16_t frame = drain_frame(s);

  EXPECT_FALSE(frame & (1u << 0));  // start = 0
  // data bits 1-8 (bit_pos 1..8 → data bits 0..7)
  EXPECT_FALSE(frame & (1u << 1));  // d0 = 0
  EXPECT_TRUE (frame & (1u << 2));  // d1 = 1
  EXPECT_FALSE(frame & (1u << 3));  // d2 = 0
  EXPECT_TRUE (frame & (1u << 4));  // d3 = 1
  EXPECT_FALSE(frame & (1u << 5));  // d4 = 0
  EXPECT_TRUE (frame & (1u << 6));  // d5 = 1
  EXPECT_FALSE(frame & (1u << 7));  // d6 = 0
  EXPECT_TRUE (frame & (1u << 8));  // d7 = 1
  EXPECT_TRUE (frame & (1u << 9));  // stop = 1
}

// ===========================================================================
// Multi-byte stream
// ===========================================================================

TEST(RawBytesTapeStream, MultiByte_ConsecutiveFrames) {
  RawBytesTapeStream s({0x01, 0x02});

  // First byte: 0x01
  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_TRUE(s.next_bit());   // d0=1
  for (int i = 1; i < 8; ++i) EXPECT_FALSE(s.next_bit());
  EXPECT_TRUE(s.next_bit());   // stop

  // Second byte: 0x02
  EXPECT_FALSE(s.next_bit());  // start
  EXPECT_FALSE(s.next_bit());  // d0=0
  EXPECT_TRUE(s.next_bit());   // d1=1
  for (int i = 2; i < 8; ++i) EXPECT_FALSE(s.next_bit());
  EXPECT_TRUE(s.next_bit());   // stop

  EXPECT_TRUE(s.end_of_tape());
}

TEST(RawBytesTapeStream, MultiByte_AtCarrierFalseOnlyAfterLast) {
  RawBytesTapeStream s({0xAA, 0xBB});
  EXPECT_TRUE(s.at_carrier());
  drain_frame(s);
  EXPECT_TRUE(s.at_carrier());  // second byte still pending
  drain_frame(s);
  EXPECT_FALSE(s.at_carrier()); // now exhausted
}

// ===========================================================================
// Recording path (write_bit / recorded_bytes)
// ===========================================================================

TEST(RawBytesTapeStream, WriteAndRead_SingleByte) {
  RawBytesTapeStream s({});
  // Write 8N1 frame for 0x55 = 0101_0101
  s.write_bit(false);  // start
  s.write_bit(true);   // d0
  s.write_bit(false);  // d1
  s.write_bit(true);   // d2
  s.write_bit(false);  // d3
  s.write_bit(true);   // d4
  s.write_bit(false);  // d5
  s.write_bit(true);   // d6
  s.write_bit(false);  // d7
  s.write_bit(true);   // stop
  ASSERT_EQ(1u, s.recorded_bytes().size());
  EXPECT_EQ(0x55, s.recorded_bytes()[0]);
}

TEST(RawBytesTapeStream, WriteAndRead_IgnoresMarkBeforeStart) {
  RawBytesTapeStream s({});
  // Mark bits before start should be ignored
  s.write_bit(true);
  s.write_bit(true);
  s.write_bit(false);  // start bit
  for (int i = 0; i < 8; ++i) s.write_bit(false);  // 0x00
  s.write_bit(true);   // stop
  ASSERT_EQ(1u, s.recorded_bytes().size());
  EXPECT_EQ(0x00, s.recorded_bytes()[0]);
}

TEST(RawBytesTapeStream, RoundTrip_MultipleBytes) {
  // Playback three bytes, feed each bit back into write_bit, confirm recording
  std::vector<uint8_t> payload = {0x41, 0x42, 0x43};  // "ABC"
  RawBytesTapeStream tx(payload);
  RawBytesTapeStream rx({});

  while (!tx.end_of_tape())
    rx.write_bit(tx.next_bit());
  // Flush the in-progress stop bit (tx is exhausted but rx may need one more bit)
  rx.write_bit(true);

  ASSERT_EQ(payload.size(), rx.recorded_bytes().size());
  for (size_t i = 0; i < payload.size(); ++i)
    EXPECT_EQ(payload[i], rx.recorded_bytes()[i]) << "byte " << i;
}
