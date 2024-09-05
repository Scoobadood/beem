#pragma once
#include "gtest/gtest.h"
#include "disassembler.h"

#include <vector>

class TestDisassembler : public ::testing::Test {
 public:
  Disassembler d_;

  std::vector<uint8_t> asla;
  std::vector<uint8_t> lda_imm_x12;
  std::vector<uint8_t> sta_abs_x1234;
  std::vector<uint8_t> adc_zp_x56;
  std::vector<uint8_t> bad_args_adc_zp_x56;
  std::vector<uint8_t> unknown;

  void SetUp();
  void TearDown();
};
