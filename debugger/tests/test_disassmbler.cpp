#include "test_disassembler.h"

#define EXPECT_THROW_WITH_MESSAGE(stmt, etype, whatstring) EXPECT_THROW( \
        try { \
            stmt; \
        } catch (const etype& ex) { \
            EXPECT_EQ(std::string(ex.what()), whatstring); \
            throw; \
        } \
    , etype)

void TestDisassembler::SetUp() {
  asla = {0x0a};
  lda_imm_x12 = {0xa9, 0x12};
  sta_abs_x1234 = {0x8d, 0x34, 0x12};
  adc_zp_x56 = {0x65, 0x56};
  bad_args_adc_zp_x56 = {0x65};
  unknown = {0x02, 0x03};
}

void TestDisassembler::TearDown() {}

/* ********************************************************************************
 * ** Test disassembl
 * ********************************************************************************/
TEST_F(TestDisassembler, ShouldFailWithEmptyData) {
  uint16_t start_offset = 0;
  uint16_t offset = start_offset;
  uint8_t err;
  auto op = d_.disassemble_one(std::vector<uint8_t>(), offset, err);
  EXPECT_EQ(offset, start_offset);
  EXPECT_EQ(err, 1);
}

TEST_F(TestDisassembler, ShouldFailWithOffsetOutOfRange) {
  auto memory = adc_zp_x56;
  uint16_t start_offset = memory.size();
  uint16_t offset = start_offset;
  uint8_t err;

  auto op = d_.disassemble_one(memory, offset, err);
  EXPECT_EQ(offset, start_offset);
  EXPECT_EQ(err, 1);
}

TEST_F(TestDisassembler, ShouldFailWithArgsOutOfRange) {
  auto memory = bad_args_adc_zp_x56;
  uint16_t start_offset = memory.size() - 1;
  uint16_t offset = start_offset;
  uint8_t err;

  auto op = d_.disassemble_one(memory, offset, err);
  EXPECT_EQ(offset, start_offset);
  EXPECT_EQ(err, 1);
}


TEST_F(TestDisassembler, ShouldCompleteWithLength1) {
  auto memory = asla;
  uint16_t start_offset = 0;
  uint16_t offset = start_offset;
  uint8_t err;

  auto op = d_.disassemble_one(memory, offset, err);
  EXPECT_EQ(op.opcode.name, "asl");
  EXPECT_EQ(op.opcode.bytes, 1);
  EXPECT_EQ(op.data, 0);
  EXPECT_EQ(offset, start_offset+1);
  EXPECT_EQ(err, 0);
}

TEST_F(TestDisassembler, ShouldCompleteWithLength2) {
  auto memory = adc_zp_x56;
  uint16_t start_offset = 0;
  uint16_t offset = start_offset;
  uint8_t err;

  auto op = d_.disassemble_one(memory, offset, err);
  EXPECT_EQ(op.opcode.name, "adc");
  EXPECT_EQ(op.opcode.addressing_mode,OpCode::ZeroPage);
  EXPECT_EQ(op.opcode.bytes, 2);
  EXPECT_EQ(op.data, 0x56);
  EXPECT_EQ(offset, start_offset+2);
  EXPECT_EQ(err, 0);
}

TEST_F(TestDisassembler, ShouldCompleteWithLength3) {
  auto memory = sta_abs_x1234;
  uint16_t start_offset = 0;
  uint16_t offset = start_offset;
  uint8_t err;

  auto op = d_.disassemble_one(memory, offset, err);
  EXPECT_EQ(op.opcode.name, "sta");
  EXPECT_EQ(op.opcode.addressing_mode,OpCode::Absolute);
  EXPECT_EQ(op.opcode.bytes, 3);
  EXPECT_EQ(op.data, 0x1234);
  EXPECT_EQ(offset, start_offset+3);
  EXPECT_EQ(err, 0);
}

TEST_F(TestDisassembler, ShouldHandleUnknownOpcode) {
  auto memory = unknown;
  uint16_t start_offset = 0;
  uint16_t offset = start_offset;
  uint8_t err;

  auto op = d_.disassemble_one(memory, offset, err);
  EXPECT_EQ(op.opcode.name, "???");
  EXPECT_EQ(op.opcode.addressing_mode,OpCode::Implied);
  EXPECT_EQ(op.opcode.bytes, 1);
  EXPECT_EQ(op.data, 0);
  EXPECT_EQ(offset, start_offset+1);
  EXPECT_EQ(err, 0);
}