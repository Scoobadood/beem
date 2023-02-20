#include "test_crtc.h"
#include "6845_crtc.h"
#include <spdlog/spdlog-inl.h>

const uint16_t CRTC_REG_SELECT = 0xfe20;
const uint16_t CRTC_READ_WRITE = 0xfe21;


/**
 * Print comparison vals as hex
 * @param expected_expr
 * @param actual_expr
 * @param expected
 * @param actual
 * @return
 */
testing::AssertionResult CmpHelperIntHex(const char* expected_expr, const char* actual_expr, int expected, int actual)
{
  if(actual == expected)
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
  if(actual_str != actual_expr) {
    msg << "\n  Actual: " << std::showbase << std::hex << actual;
  }
  msg << "\nExpected: " << expected_expr;
  if(expected_str != expected_expr) {
    msg << "\nWhich is: " << std::showbase << std::hex << expected;
  }

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
  set_register(3, 0x42);
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
  std::vector<uint16_t> expected_addresses{
          0x5800, 0x5808, 0x5810, 0x5818, 0x5820, 0x5828, 0x5830, 0x5838,
          0x5840, 0x5848, 0x5850, 0x5858, 0x5860, 0x5868, 0x5870, 0x5878,
          0x5880, 0x5888, 0x5890, 0x5898, 0x58a0, 0x58a8, 0x58b0, 0x58b8,
          0x58c0, 0x58c8, 0x58d0, 0x58d8, 0x58e0, 0x58e8, 0x58f0, 0x58f8,
          0x5900, 0x5908, 0x5910, 0x5918, 0x5920, 0x5928, 0x5930, 0x5938,
          // DE should go off here
          0x5940, 0x5948, 0x5950, 0x5958, 0x5960, 0x5968, 0x5970, 0x5978,
          0x5980, /*HS*/  0x5988, 0x5990, 0x5998, 0x59a0, /*HS end*/ 0x59a8, 0x59b0, 0x59b8,
          0x59c0, 0x59c8, 0x59d0, 0x59d8, 0x59e0, 0x59e8, 0x59f0, 0x59f8,

          // New Raster Here, DE back on
          0x5801, 0x5809, 0x5811, 0x5819, 0x5821, 0x5829, 0x5831, 0x5839,
          0x5841, 0x5849, 0x5851, 0x5859, 0x5861, 0x5869, 0x5871, 0x5879,
          0x5881, 0x5889, 0x5891, 0x5899, 0x58a1, 0x58a9, 0x58b1, 0x58b9,
          0x58c1, 0x58c9, 0x58d1, 0x58d9, 0x58e1, 0x58e9, 0x58f1, 0x58f9,
          0x5901, 0x5909, 0x5911, 0x5919, 0x5921, 0x5929, 0x5931, 0x5939,
          // DE should go off here
          0x5941, 0x5949, 0x5951, 0x5959, 0x5961, 0x5969, 0x5971, 0x5979,
          0x5981, /*HS*/  0x5989, 0x5991, 0x5999, 0x59a1, /*HS end*/ 0x59a9, 0x59b1, 0x59b9,
          0x59c1, 0x59c9, 0x59d1, 0x59d9, 0x59e1, 0x59e9, 0x59f1, 0x59f9
  };

  for( auto expected : expected_addresses) {
    crtc_->generate_next_address(dram_bus_);
    auto actual = dram_bus_->get_address();

    EXPECT_PRED_FORMAT2(CmpHelperIntHex, expected, actual);
  }
}
