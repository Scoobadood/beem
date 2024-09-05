#include "test_clock.h"
#include <gtest/gtest.h>

void TestClock::SetUp() {
  clock = new Clock();
}

void TestClock::TearDown() {
  delete clock;
}

void TestClock::tick(uint32_t n) {
  while (n-- != 0) {
    clock->tick();
  }
}

/*
 * Test that the 16MHz clock starts high
 */
TEST_F(TestClock, clk_16mz_starts_low) {
  EXPECT_TRUE(clock->is_low(CLK_16_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_16_MHZ));
}
/*
 * Test that the 16MHz clock toggles each tick
 */
TEST_F(TestClock, clk_16mz_toggles_each_tick) {
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(clock->went_low(CLK_16_MHZ));
    clock->tick();

    EXPECT_TRUE(clock->went_high(CLK_16_MHZ));
    clock->tick();
  }
}

/*
 * Test that the 8MHz starts low and toggles every other tick
 */
TEST_F(TestClock, clk_8mz_toggles_each_second_tick) {
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(clock->is_low(CLK_8_MHZ));
    EXPECT_TRUE(clock->went_low(CLK_8_MHZ));
    clock->tick();

    EXPECT_TRUE(clock->is_low(CLK_8_MHZ));
    EXPECT_FALSE(clock->went_low(CLK_8_MHZ));
    clock->tick();

    EXPECT_TRUE(clock->is_high(CLK_8_MHZ));
    EXPECT_TRUE(clock->went_high(CLK_8_MHZ));
    clock->tick();

    EXPECT_TRUE(clock->is_high(CLK_8_MHZ));
    EXPECT_FALSE(clock->went_high(CLK_8_MHZ));
    clock->tick();
  }
}

/*
 * Test that the 4MHz starts low and toggles every fourth tick
 */
TEST_F(TestClock, clk_4mz_toggles_each_fourth_tick) {
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(clock->is_low(CLK_4_MHZ));
    EXPECT_TRUE(clock->went_low(CLK_4_MHZ));
    clock->tick();

    for (int j = 0; j < 3; ++j) {
      EXPECT_TRUE(clock->is_low(CLK_4_MHZ));
      EXPECT_FALSE(clock->went_low(CLK_4_MHZ));
      clock->tick();
    }

    EXPECT_TRUE(clock->is_high(CLK_4_MHZ));
    EXPECT_TRUE(clock->went_high(CLK_4_MHZ));
    clock->tick();

    for (int j = 0; j < 3; ++j) {
      EXPECT_TRUE(clock->is_high(CLK_4_MHZ));
      EXPECT_FALSE(clock->went_high(CLK_4_MHZ));
      clock->tick();
    }
  }
}

/*
 * Test that the 2MHz starts high and toggles every eighth tick
 */
TEST_F(TestClock, clk_2mz_toggles_each_eighth_tick) {
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
    EXPECT_TRUE(clock->went_low(CLK_2_MHZ));
    clock->tick();

    for (int j = 0; j < 7; ++j) {
      EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
      EXPECT_FALSE(clock->went_low(CLK_2_MHZ));
      clock->tick();
    }

    EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
    EXPECT_TRUE(clock->went_high(CLK_2_MHZ));
    clock->tick();

    for (int j = 0; j < 7; ++j) {
      EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
      EXPECT_FALSE(clock->went_high(CLK_2_MHZ));
      clock->tick();
    }
  }
}

/*
 * Test that the 2MHzE starts high and toggles every eighth tick
 */
TEST_F(TestClock, clk_2mze_toggles_each_eighth_tick) {
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
    EXPECT_TRUE(clock->went_low(CLK_2_MHZ));
    clock->tick();

    for (int j = 0; j < 7; ++j) {
      EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
      EXPECT_FALSE(clock->went_low(CLK_2_MHZ));
      clock->tick();
    }

    EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
    EXPECT_TRUE(clock->went_high(CLK_E_2_MHZ));
    clock->tick();

    for (int j = 0; j < 7; ++j) {
      EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
      EXPECT_FALSE(clock->went_high(CLK_E_2_MHZ));
      clock->tick();
    }
  }
}

/*
 * Test that the 1MHz starts high and toggles every 16th tick
 */
TEST_F(TestClock, clk_1mz_toggles_each_sixteenth_tick) {
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
    EXPECT_TRUE(clock->went_low(CLK_1_MHZ));
    clock->tick();

    for (int j = 0; j < 15; ++j) {
      EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
      EXPECT_FALSE(clock->went_low(CLK_1_MHZ));
      clock->tick();
    }

    EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
    EXPECT_TRUE(clock->went_high(CLK_1_MHZ));
    clock->tick();

    for (int j = 0; j < 15; ++j) {
      EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
      EXPECT_FALSE(clock->went_high(CLK_1_MHZ));
      clock->tick();
    }
  }
}

/*
 * If 1MHz and 2MHZE are out of phase, stretching should look like this
 *
 * 1MHz
 *          +-A-------+         +---------+         +---------+         +---------+         +---------+
 *          | :       |         |         |         |         |         |         |         |         |
 * ---------+ :       +---------+         +---------+         +---------+         +---------+         +
 *            :  Stretch RQ arrives when 1MHz
 *            :  and 2MHzE are out of phase
 *            :
 *            A  B    C    D    E    F    G
 *     +----+ :  :    :    +--------------+    +----+    +----+    +----+    +----+    +----+    +----+
 *     |    | :  :    :    |    :    :    |    |    |    |    |    |    |    |    |    |    |    |    |
 * ----+    +-A--:----:----+    :    :    +----+    +----+    +----+    +----+    +----+    +----+    +----+
 * 2MHzE      :  :    :    :    :    :
 *            :  :    :    :    :    :
 *     +----+ :  +----+    +----+    +----+    +----+    +----+    +----+    +----+    +----+    +----+
 *     |    | :  |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
 * ----+    +-A--+    +----+    +----+    +----+    +----+    +----+    +----+    +----+    +----+    +----+
 * 2MHz ref
 */
TEST_F(TestClock, slowed_clock_responds_correctly_oop) {
  tick(16);
  // A
  EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));

  clock->begin_time_stretch();

  tick(8);

  // B
  EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
  EXPECT_FALSE(clock->went_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_2_MHZ));
  tick(8);

  // C
  EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
  EXPECT_FALSE(clock->went_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));
  tick(8);

  // D
  EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
  EXPECT_FALSE(clock->went_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_2_MHZ));
  tick(8);

  // E
  EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));
  tick(8);

  // F
  EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_2_MHZ));
  tick(8);

  // G
  EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));

  EXPECT_FALSE( clock->is_stretched());
}

/*
 * If 1MHz and 2MHZE are out of phase, stretching should look like this
 *
 * 1MHz
 *          +---------+         +---------+         +---------+         +---------+         +---------+
 *          |         |         |         |         |         |         |         |         |         |
 * ---------+         +-A-------+         +---------+         +---------+         +---------+         +
 *   Stretch RQ arrives :  :    :    :    :
 *  when 1MHz and 2MHzE :  :    :    :    :
 *     are out of phase :  :    :    :    :
 *                      :  :    :    :    :
 *                      :  B    C    D    E
 *     +----+    +----+ :  +--------------+    +----+    +----+    +----+    +----+    +----+    +----+
 *     |    |    |    | :  |              |    |    |    |    |    |    |    |    |    |    |    |    |
 * ----+    +----+    +-A--+              +----+    +----+    +----+    +----+    +----+    +----+    +----+
 * 2MHzE         :    : :  :    :    :    :
 *               :    : :  :    :    :    :
 *     +----+    +----+ :  +----+    +----+    +----+    +----+    +----+    +----+    +----+    +----+
 *     |    |    |    | :  |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
 * ----+    +----+    +-A--+    +----+    +----+    +----+    +----+    +----+    +----+    +----+    +----+
 * 2MHz ref
 */
TEST_F(TestClock, slowed_clock_responds_correctly_ip) {
  // Wait 32 clocks
  tick(32);

  // A
  EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));

  // Begin in phase stretch
  clock->begin_time_stretch();

  tick(8);

  // B
  EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
  EXPECT_FALSE(clock->went_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_2_MHZ));

  tick(8);

  // C
  EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));
  tick(8);

  // D
  EXPECT_TRUE(clock->is_high(CLK_1_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_E_2_MHZ));
  EXPECT_FALSE(clock->went_high(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_high(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_high(CLK_2_MHZ));
  tick(8);

  // E
  EXPECT_TRUE(clock->is_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_1_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_E_2_MHZ));
  EXPECT_TRUE(clock->is_low(CLK_2_MHZ));
  EXPECT_TRUE(clock->went_low(CLK_2_MHZ));
  tick(8);

  EXPECT_FALSE( clock->is_stretched());
}

