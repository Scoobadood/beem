#ifndef BEEB_HARDWARE_CRTC_TEST_MODE_0_H_
#define BEEB_HARDWARE_CRTC_TEST_MODE_0_H_

#include "gtest/gtest.h"
#include "6845_crtc.h"

class TestMode0 : public ::testing::Test {
 public:
  void SetUp( );
  void TearDown( );

  Crtc * crtc;
  Bus bus;
};

#endif // BEEB_HARDWARE_CRTC_TEST_MODE_0_H_
