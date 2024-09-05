#ifndef BEEB_HARDWARE_CRTC_TEST_MODE_0_H_
#define BEEB_HARDWARE_CRTC_TEST_MODE_0_H_

#include <gtest/gtest.h>
#include "beeb.h"

class TestBeebMemory : public ::testing::Test {
 public:
  void SetUp( );
  void TearDown( );

  std::shared_ptr<Beeb> beeb;

};

#endif // BEEB_HARDWARE_CRTC_TEST_MODE_0_H_
