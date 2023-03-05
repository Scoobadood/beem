#ifndef BEEB_HARDWARE_CRT_INCLUDE_H
#define BEEB_HARDWARE_CRT_INCLUDE_H

#include "6845_crtc.h"
#include "5c094_vula.h"

using Renderer = std::function<void(int32_t w, int32_t h, std::vector<uint8_t> scr_data)>;

class Crt {
public:
  Crt(const std::shared_ptr<Crtc> &crtc,
      const std::shared_ptr<VideoUla> &v_ula);

  void tick();

  void set_renderer(const Renderer &renderer) {
    renderer_ = renderer;
  }

  static const int WIDTH = 1024; // 128 * 8
  static const int HEIGHT = 625; // (39 * 8 +0.5) * 2

private:
  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<VideoUla> v_ula_;
  std::vector<uint8_t> screen_data_;
  Renderer renderer_;
  int32_t pixel_x_;
  int32_t pixel_y_;
  bool last_vs_;
  bool last_hs_;
  bool even_frame_;
  uint32_t warm_up_;
  bool ready_;
};

#endif // BEEB_HARDWARE_CRT_INCLUDE_H
