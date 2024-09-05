#include "TestKeyboardLEDs.h"

void TestKeyboardLEDs::SetUp() {
  keyboard_ = new Keyboard();
}

void TestKeyboardLEDs::TearDown() {
  delete keyboard_;
}

TEST_F(TestKeyboardLEDs, CapsLockTurnsOnWithCorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->caps_lock_led());
  keyboard_->cl_led_src()->set_data(6, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->caps_lock_led());
}

TEST_F(TestKeyboardLEDs, CapsLockTurnsOffWithCorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->caps_lock_led());
  keyboard_->cl_led_src()->set_data(6, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->caps_lock_led());

  keyboard_->cl_led_src()->set_data(6, false);
  keyboard_->tick();
  EXPECT_FALSE(keyboard_->caps_lock_led());

}

TEST_F(TestKeyboardLEDs, ShiftLockTurnsOffWithCorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->shift_lock_led());
  keyboard_->sl_led_src()->set_data(7, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->shift_lock_led());

  keyboard_->sl_led_src()->set_data(7, false);
  keyboard_->tick();
  EXPECT_FALSE(keyboard_->shift_lock_led());

}

TEST_F(TestKeyboardLEDs, CapsLockStaysOffWithIncorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->caps_lock_led());
  for( int i=0; i<8; ++i ) {
    if( i == 6) continue;
    keyboard_->cl_led_src()->set_data(i, true);
    keyboard_->tick();
    EXPECT_FALSE(keyboard_->caps_lock_led());
  }
}

TEST_F(TestKeyboardLEDs, ShiftLockTurnsOnWithCorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->shift_lock_led());
  keyboard_->sl_led_src()->set_data(7, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->shift_lock_led());
}

TEST_F(TestKeyboardLEDs, ShiftLockStaysOffWithIncorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->shift_lock_led());
  for( int i=0; i<8; ++i ) {
    if( i == 7) continue;
    keyboard_->sl_led_src()->set_data(i, true);
    keyboard_->tick();
    EXPECT_FALSE(keyboard_->shift_lock_led());
  }
}

TEST_F(TestKeyboardLEDs, ShiftLockStaysOnWithIncorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->shift_lock_led());
  keyboard_->sl_led_src()->set_data(7, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->shift_lock_led());

  for( int i=0; i<8; ++i ) {
    if( i == 7) continue;
    keyboard_->sl_led_src()->set_data(i, false);
    keyboard_->tick();
    EXPECT_TRUE(keyboard_->shift_lock_led());
  }
}

TEST_F(TestKeyboardLEDs, CapsLockStaysOnWithIncorrectPIN) {
  using namespace std;

  EXPECT_FALSE(keyboard_->caps_lock_led());
  keyboard_->cl_led_src()->set_data(6, true);
  keyboard_->tick();
  EXPECT_TRUE(keyboard_->caps_lock_led());

  for( int i=0; i<8; ++i ) {
    if( i == 6) continue;
    keyboard_->cl_led_src()->set_data(i, false);
    keyboard_->tick();
    EXPECT_TRUE(keyboard_->caps_lock_led());
  }
}
