#ifndef BEEB_TEST_CRTC_H
#define BEEB_TEST_CRTC_H

#include <gtest/gtest.h>
#include "6845_crtc.h"

class TestCrtc : public testing::Test {
  void SetUp() override;

  std::shared_ptr<Crtc> crtc_;
};

#endif //BEEB_TEST_CRTC_H
