#include <gtest/gtest.h>
#include "cassette_player.h"
#include "raw_bytes_tape_stream.h"

// ===========================================================================
// Motor off — all outputs idle regardless of stream
// ===========================================================================

TEST(CassettePlayer, MotorOff_RxDataReturnsMark) {
  RawBytesTapeStream stream({0x00});
  CassettePlayer player;
  player.set_stream(&stream);
  // motor is off by default
  EXPECT_TRUE(player.rx_data());
}

TEST(CassettePlayer, MotorOff_HasCarrierFalse) {
  RawBytesTapeStream stream({0xFF});
  CassettePlayer player;
  player.set_stream(&stream);
  EXPECT_FALSE(player.has_carrier());
}

TEST(CassettePlayer, MotorOff_TxBitDoesNotWriteToStream) {
  RawBytesTapeStream stream({});
  CassettePlayer player;
  player.set_stream(&stream);
  // Drive a full 8N1 frame with motor off — nothing should be recorded
  player.tx_bit(false);
  for (int i = 0; i < 9; ++i) player.tx_bit(false);
  player.tx_bit(true);
  EXPECT_TRUE(stream.recorded_bytes().empty());
}

// ===========================================================================
// No stream — safe with motor on
// ===========================================================================

TEST(CassettePlayer, NoStream_RxDataReturnsMark) {
  CassettePlayer player;
  player.set_motor(true);
  EXPECT_TRUE(player.rx_data());
}

TEST(CassettePlayer, NoStream_HasCarrierFalse) {
  CassettePlayer player;
  player.set_motor(true);
  EXPECT_FALSE(player.has_carrier());
}

TEST(CassettePlayer, NoStream_TxBitNoCrash) {
  CassettePlayer player;
  player.set_motor(true);
  player.tx_bit(true);  // must not crash
}

// ===========================================================================
// Motor on — delegates to stream
// ===========================================================================

TEST(CassettePlayer, MotorOn_RxDataAdvancesStream) {
  // 0x01 = start(0) d0(1) d1-d7(0) stop(1)
  RawBytesTapeStream stream({0x01});
  CassettePlayer player;
  player.set_stream(&stream);
  player.set_motor(true);

  EXPECT_FALSE(player.rx_data());  // start bit
  EXPECT_TRUE(player.rx_data());   // d0 = 1
  for (int i = 1; i < 8; ++i)
    EXPECT_FALSE(player.rx_data()) << "bit " << i;
  EXPECT_TRUE(player.rx_data());   // stop bit
  EXPECT_TRUE(stream.end_of_tape());
}

TEST(CassettePlayer, MotorOn_HasCarrierDelegates) {
  RawBytesTapeStream stream({0xAA});
  CassettePlayer player;
  player.set_stream(&stream);
  player.set_motor(true);
  EXPECT_TRUE(player.has_carrier());
}

TEST(CassettePlayer, MotorOn_HasCarrierFalseAtEndOfTape) {
  RawBytesTapeStream stream({0x00});
  CassettePlayer player;
  player.set_stream(&stream);
  player.set_motor(true);
  // Drain the tape
  while (!stream.end_of_tape()) player.rx_data();
  EXPECT_FALSE(player.has_carrier());
}

TEST(CassettePlayer, MotorOn_TxBitForwardedToStream) {
  RawBytesTapeStream stream({});
  CassettePlayer player;
  player.set_stream(&stream);
  player.set_motor(true);
  // Write 8N1 frame for 0x55
  player.tx_bit(false);           // start
  for (int i = 0; i < 8; ++i)
    player.tx_bit((0x55 >> i) & 1);
  player.tx_bit(true);            // stop
  ASSERT_EQ(1u, stream.recorded_bytes().size());
  EXPECT_EQ(0x55, stream.recorded_bytes()[0]);
}

// ===========================================================================
// Motor transitions
// ===========================================================================

TEST(CassettePlayer, MotorOn_ThenOff_StopsStream) {
  RawBytesTapeStream stream({0xFF, 0xFF});
  CassettePlayer player;
  player.set_stream(&stream);

  player.set_motor(true);
  player.rx_data();  // consumes one bit
  player.set_motor(false);
  // Should not consume any more bits while motor is off
  for (int i = 0; i < 20; ++i) player.rx_data();

  // Only one bit was consumed before motor off
  // The stream TX byte should still be 0 (byte not complete)
  EXPECT_FALSE(stream.end_of_tape());
}

TEST(CassettePlayer, SetMotor_TracksState) {
  CassettePlayer player;
  EXPECT_FALSE(player.motor_on());
  player.set_motor(true);
  EXPECT_TRUE(player.motor_on());
  player.set_motor(false);
  EXPECT_FALSE(player.motor_on());
}
