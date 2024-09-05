#ifndef BEEB_TEST_CRTC_H
#define BEEB_TEST_CRTC_H

#include <gtest/gtest.h>
#include "6845_crtc.h"

class TestCrtc : public testing::Test {
public:
  void SetUp() override;

  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<Bus> bus_;
  std::shared_ptr<Bus> dram_bus_;

  void set_register(uint8_t reg, uint8_t value) const;
};

#endif //BEEB_TEST_CRTC_H
