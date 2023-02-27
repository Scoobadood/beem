#include "crt.h"

Crt::Crt(const std::shared_ptr<Crtc> &crtc,
         const std::shared_ptr<VideoUla> &v_ula) //
        : crtc_{crtc} //
        , v_ula_{v_ula} //
        , renderer_{nullptr} //
{
  screen_data_.reserve(1280 * 768 * 3);
  pixel_x_ = 0;
  pixel_y_ = 0;
}

void
Crt::update_screen() {
  static bool last_vs;
  static bool last_hs;

  auto vs = crtc_->vsync();
  auto hs = crtc_->hsync();

  // HS just happened. Inc pix y and reset pix x
  if (!hs && last_hs) {
//    pixel_x_ = 0;
//    ++pixel_y_;
  }

  // VS just happened.
  // Send the screen and reset pix x and pix y
  if (!vs && last_vs) {
    renderer_(1024, 312, screen_data_);
    pixel_x_ = 0;
    pixel_y_ = 0;
    screen_data_.clear();
  }

  if (++pixel_x_ == 1024) {
    pixel_x_ = 0;
    if (++pixel_y_ == 312) pixel_y_ = 0;
  }


  uint32_t pixel_colour = 0;
  if (crtc_->display_enable()) {
    pixel_colour = v_ula_->rgb();
  }
  screen_data_.push_back((pixel_colour >> 16) & 0xff);
  screen_data_.push_back((pixel_colour >> 8) & 0xff);
  screen_data_.push_back((pixel_colour >> 0) & 0xff);

  last_vs = vs;
  last_hs = hs;
}