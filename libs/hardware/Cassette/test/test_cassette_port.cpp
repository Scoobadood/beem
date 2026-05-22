#include <gtest/gtest.h>
#include "cassette_port.h"
#include "raw_bytes_tape_stream.h"

// ===========================================================================
// Motor off — all outputs idle regardless of stream
// ===========================================================================

TEST(CassettePort, MotorOff_RxDataReturnsMark) {
  RawBytesTapeStream stream({0x00});
  CassettePort port;
  port.set_stream(&stream);
  // motor is off by default
  EXPECT_TRUE(port.rx_data());
}

TEST(CassettePort, MotorOff_HasCarrierFalse) {
  RawBytesTapeStream stream({0xFF});
  CassettePort port;
  port.set_stream(&stream);
  EXPECT_FALSE(port.has_carrier());
}

TEST(CassettePort, MotorOff_TxBitDoesNotWriteToStream) {
  RawBytesTapeStream stream({});
  CassettePort port;
  port.set_stream(&stream);
  // Drive a full 8N1 frame with motor off — nothing should be recorded
  port.tx_bit(false);
  for (int i = 0; i < 9; ++i) port.tx_bit(false);
  port.tx_bit(true);
  EXPECT_TRUE(stream.recorded_bytes().empty());
}

// ===========================================================================
// No stream — safe with motor on
// ===========================================================================

TEST(CassettePort, NoStream_RxDataReturnsMark) {
  CassettePort port;
  port.set_motor(true);
  EXPECT_TRUE(port.rx_data());
}

TEST(CassettePort, NoStream_HasCarrierFalse) {
  CassettePort port;
  port.set_motor(true);
  EXPECT_FALSE(port.has_carrier());
}

TEST(CassettePort, NoStream_TxBitNoCrash) {
  CassettePort port;
  port.set_motor(true);
  port.tx_bit(true);  // must not crash
}

// ===========================================================================
// Motor on — delegates to stream
// ===========================================================================

TEST(CassettePort, MotorOn_RxDataAdvancesStream) {
  // 0x01 = start(0) d0(1) d1-d7(0) stop(1)
  RawBytesTapeStream stream({0x01});
  CassettePort port;
  port.set_stream(&stream);
  port.set_motor(true);

  EXPECT_FALSE(port.rx_data());  // start bit
  EXPECT_TRUE(port.rx_data());   // d0 = 1
  for (int i = 1; i < 8; ++i)
    EXPECT_FALSE(port.rx_data()) << "bit " << i;
  EXPECT_TRUE(port.rx_data());   // stop bit
  EXPECT_TRUE(stream.end_of_tape());
}

TEST(CassettePort, MotorOn_HasCarrierDelegates) {
  RawBytesTapeStream stream({0xAA});
  CassettePort port;
  port.set_stream(&stream);
  port.set_motor(true);
  EXPECT_TRUE(port.has_carrier());
}

TEST(CassettePort, MotorOn_HasCarrierFalseAtEndOfTape) {
  RawBytesTapeStream stream({0x00});
  CassettePort port;
  port.set_stream(&stream);
  port.set_motor(true);
  // Drain the tape
  while (!stream.end_of_tape()) port.rx_data();
  EXPECT_FALSE(port.has_carrier());
}

TEST(CassettePort, MotorOn_TxBitForwardedToStream) {
  RawBytesTapeStream stream({});
  CassettePort port;
  port.set_stream(&stream);
  port.set_motor(true);
  // Write 8N1 frame for 0x55
  port.tx_bit(false);           // start
  for (int i = 0; i < 8; ++i)
    port.tx_bit((0x55 >> i) & 1);
  port.tx_bit(true);            // stop
  ASSERT_EQ(1u, stream.recorded_bytes().size());
  EXPECT_EQ(0x55, stream.recorded_bytes()[0]);
}

// ===========================================================================
// Motor transitions
// ===========================================================================

TEST(CassettePort, MotorOn_ThenOff_StopsStream) {
  RawBytesTapeStream stream({0xFF, 0xFF});
  CassettePort port;
  port.set_stream(&stream);

  port.set_motor(true);
  port.rx_data();  // consumes one bit
  port.set_motor(false);
  // Should not consume any more bits while motor is off
  for (int i = 0; i < 20; ++i) port.rx_data();

  EXPECT_FALSE(stream.end_of_tape());
}

TEST(CassettePort, SetMotor_TracksState) {
  CassettePort port;
  EXPECT_FALSE(port.motor_on());
  port.set_motor(true);
  EXPECT_TRUE(port.motor_on());
  port.set_motor(false);
  EXPECT_FALSE(port.motor_on());
}
