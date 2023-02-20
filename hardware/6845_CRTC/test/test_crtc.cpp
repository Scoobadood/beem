#include "test_crtc.h"
#include "6845_crtc.h"

void TestCrtc::SetUp() {
  crtc_ = std::make_shared<Crtc>(0xfe20);
}

TEST(TestCrtc, test_mode_4_sequence) {
}