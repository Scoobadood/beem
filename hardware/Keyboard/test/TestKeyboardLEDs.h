//
// Created by Dave Durbin on 12/1/2023.
//

#ifndef BEEB_KEYBOARD_TEST_LEDS_H_
#define BEEB_KEYBOARD_TEST_LEDS_H_

#include "gtest/gtest.h"
#include "keyboard.h"

class TestKeyboardLEDs : public ::testing::Test {
 public:
  void SetUp( );
  void TearDown( );

  Keyboard * keyboard_;
};

#endif // BEEB_KEYBOARD_TEST_LEDS_H_
