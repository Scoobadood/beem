#ifndef BEEB_KEYBOARD_TEST_AUTOSCAN_H_
#define BEEB_KEYBOARD_TEST_AUTOSCAN_H_

#include "gtest/gtest.h"
#include "keyboard.h"

class TestKeyboardAutoScan : public ::testing::Test {
 public:
  void SetUp( );
  void TearDown( );

  Keyboard * keyboard_;
};

#endif // BEEB_KEYBOARD_TEST_AUTOSCAN_H_
