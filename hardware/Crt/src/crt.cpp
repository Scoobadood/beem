#include "crt.h"
#include "spdlog/spdlog.h"


Crt::Crt(const std::shared_ptr<Crtc> &crtc,
         const std::shared_ptr<VideoUla> &v_ula) //
        : crtc_{crtc} //
        , v_ula_{v_ula} //
        , renderer_{nullptr} //
        , pixel_x_{0} //
        , pixel_y_{0} //
        , last_vs_{false} //
        , last_hs_{false} //
{
  screen_data_.resize(1024 * 768 * 3, 0);
  even_frame_ = true;
  warm_up_ = std::numeric_limits<uint16_t>::max() * 4;
  ready_ = false;
}


void
Crt::tick() {
  if( !ready_) {
    if( warm_up_-- == 0) ready_ = true;
    return;
  }

  auto vs = crtc_->vsync();
  auto hs = crtc_->hsync();

  uint32_t pixel_colour = 0x808080;
  if (vs) {
      pixel_colour |= 0xff0000;
  }
  if (hs) {
    pixel_colour |= 0x00ff00;
  }
  if (!hs && !vs && crtc_->display_enable()) {
    pixel_colour = v_ula_->rgb();
  }
  auto pixel_addr_ = ((pixel_y_ * 1024) + pixel_x_) * 3;
  if( pixel_addr_ >=0) {
    screen_data_.at(pixel_addr_) = ((pixel_colour >> 16) & 0xff);
    screen_data_.at(pixel_addr_ + 1) = ((pixel_colour >> 8) & 0xff);
    screen_data_.at(pixel_addr_ + 2) = ((pixel_colour >> 0) & 0xff);
  }

  ++pixel_x_;

  if (hs && !last_hs_) {
    pixel_x_ = 0;
    pixel_y_ += 2;
  }

  if (vs && !last_vs_) {
    pixel_x_ = even_frame_ ? 0 : pixel_x_;
    pixel_y_ = even_frame_ ? -1 : 0;
    even_frame_ = !even_frame_;
    renderer_(1024, 768, screen_data_);
  }

  // Flyback at 768 anyway
//  if( pixel_x_ >=1024) {
//    spdlog::error( "PixelX is {}. Expected hsync before now. Manually resetting", pixel_x_);
////    pixel_x_ = 0;
//  }
  if (pixel_y_ >= 768) {
//    spdlog::error( "PixelY is {}. Expected vsync before now. Manually resetting", pixel_y_);
    pixel_x_ = pixel_y_ = 0;
    even_frame_ = true;
    renderer_(1024, 768, screen_data_);
  }
  last_hs_ = hs;
  last_vs_ = vs;
}