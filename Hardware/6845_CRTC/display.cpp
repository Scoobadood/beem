//
// Created by Dave Durbin on 30/12/2022.
//

#include "display.h"

Display::Display() {
  mode_ = 7;
  pixel_data_rgb_ = new uint8_t[480 * 500 * 3];
  memset(pixel_data_rgb_, 0, 480 * 500 * 3);
}

Display::~Display() {
  delete[] pixel_data_rgb_;
}
