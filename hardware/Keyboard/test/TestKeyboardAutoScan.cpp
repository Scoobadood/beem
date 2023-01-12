#include "TestKeyboardAutoScan.h"

void TestKeyboardAutoScan::SetUp() {
  keyboard_ = new Keyboard();
}

void TestKeyboardAutoScan::TearDown() {
  delete keyboard_;
}

TEST_F(TestKeyboardAutoScan, AutoScanEnabledByDefault) {
  EXPECT_TRUE(keyboard_->auto_scan_enabled());
}

TEST_F(TestKeyboardAutoScan, AutoScanDisableWithCorrectPIN) {
  keyboard_->we_src()->set_data(3, false);
  keyboard_->tick();
  EXPECT_FALSE(keyboard_->auto_scan_enabled());
}

TEST_F(TestKeyboardAutoScan, AutoScanRemainsOnWithIncorrectPIN) {
  for (int i = 0; i < 8; ++i) {
    if (i == 3) continue;
    keyboard_->we_src()->set_data(i, false);
    keyboard_->tick();
    EXPECT_TRUE(keyboard_->auto_scan_enabled());
  }
}

TEST_F(TestKeyboardAutoScan, AutoScanEnableWithCorrectPIN) {
  keyboard_->we_src()->set_data(3, false);
  keyboard_->tick();
  EXPECT_FALSE(keyboard_->auto_scan_enabled());

  keyboard_->we_src()->set_data(3, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->auto_scan_enabled());
}

TEST_F(TestKeyboardAutoScan, AutoScanRemainsOffWithIncorrectPIN) {
  keyboard_->we_src()->set_data(3, false);
  keyboard_->tick();
  EXPECT_FALSE(keyboard_->auto_scan_enabled());

  for (int i = 0; i < 8; ++i) {
    if (i == 3) continue;
    keyboard_->we_src()->set_data(i, true);
    keyboard_->tick();
    EXPECT_FALSE(keyboard_->auto_scan_enabled());
  }
}

