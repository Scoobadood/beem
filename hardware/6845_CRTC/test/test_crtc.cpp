#include "test_crtc.h"
#include "6845_crtc.h"
#include <spdlog/spdlog-inl.h>

#include "mode_4_data.h"

const uint16_t CRTC_REG_SELECT = 0xfe00;
const uint16_t CRTC_READ_WRITE = 0xfe01;


/**
 * Print comparison vals as hex
 * @param expected_expr
 * @param actual_expr
 * @param expected
 * @param actual
 * @return
 */
testing::AssertionResult CmpHelperIntHex(const char *expected_expr, const char *actual_expr, int expected, int actual) {
  if (actual == expected)
    return testing::AssertionSuccess();

  std::stringstream ss, msg;
  std::string expected_str, actual_str;

  ss.str("");
  ss << std::showbase << std::hex << expected;
  expected_str = ss.str();

  ss.str("");
  ss << std::showbase << std::hex << actual;
  actual_str = ss.str();

  msg << "Value of: " << actual_expr;
  if (actual_str != actual_expr) {
    msg << "\n  Actual: " << std::showbase << std::hex << actual;
  }
  msg << "\nExpected: " << expected_expr;
  if (expected_str != expected_expr) {
    msg << "\nWhich is: " << std::showbase << std::hex << expected;
  }

  return testing::AssertionFailure() << msg.str();
}


testing::AssertionResult CmpHelperCharPos(const char *expected_expr, const char *actual_expr, const char *char_pos_expr, bool expected, bool actual, int charpos) {
  if (actual == expected)
    return testing::AssertionSuccess();

  std::stringstream ss, msg;
  msg << "Expected " <<  actual_expr << " at " << charpos << " to be " << ((expected) ? "true" : "false");
  msg << "\n  Actual: " << ((actual) ? "true" : "false");
return testing::AssertionFailure() << msg.str();
}

void TestCrtc::set_register(uint8_t reg, uint8_t value) const {
  bus_->clr_RW();
  bus_->set_address(CRTC_REG_SELECT);
  bus_->set_data(reg);
  crtc_->tick(bus_);

  bus_->set_address(CRTC_READ_WRITE);
  bus_->set_data(value);
  crtc_->tick(bus_);
}

void TestCrtc::SetUp() {
  try {
    std::vector<spdlog::sink_ptr> sinks;
    auto logger = std::make_shared<spdlog::logger>("BusDance", begin(sinks), end(sinks));
    spdlog::register_logger(logger);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }


  crtc_ = std::make_shared<Crtc>(CRTC_REG_SELECT);
  bus_ = std::make_shared<Bus>();
  dram_bus_ = std::make_shared<Bus>();
  set_register(0, 0x3f);
  set_register(1, 0x28);
  set_register(2, 0x31);
  set_register(3, 0x24);
  set_register(4, 0x26);
  set_register(5, 0x00);
  set_register(6, 0x20);
  set_register(7, 0x22);
  set_register(8, 0x01);
  set_register(9, 0x07);
  set_register(10, 0x67);
  set_register(11, 0x08);
  set_register(12, 0x0b);
  set_register(13, 0x00);
  set_register(14, 0x0b);
  set_register(15, 0x00);
}

TEST_F(TestCrtc, test_mode_4_sequence) {
  for (auto expected: expected_addresses) {
    crtc_->generate_next_address(dram_bus_);
    auto actual = dram_bus_->get_address();

    EXPECT_PRED_FORMAT2(CmpHelperIntHex, expected, actual);
  }
}

TEST_F(TestCrtc, test_mode_4_hsync) {
  for (auto scanline = 0; scanline < 320; ++scanline) {
    for (auto charpos = 0; charpos < 64; ++charpos) {
      crtc_->generate_next_address(dram_bus_);
      auto hs = crtc_->hsync();
      if (charpos < 49 || charpos > 52) {
        EXPECT_PRED_FORMAT3(CmpHelperCharPos, false, hs, charpos);
      } else {
        EXPECT_PRED_FORMAT3(CmpHelperCharPos, true, hs, charpos);
      }
    }
  }
}

