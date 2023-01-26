#ifndef BEEB_TEST_CLOCK_H
#define BEEB_TEST_CLOCK_H

#include "clock.h"
#include <gtest/gtest.h>

class TestClock : public ::testing::Test {
public:
  void SetUp() override;

  void TearDown() override;

  void tick(uint32_t n);

  Clock *clock;
};


#endif //BEEB_TEST_CLOCK_H
