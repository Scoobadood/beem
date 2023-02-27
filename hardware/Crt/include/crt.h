#ifndef BEEB_HARDWARE_CRT_INCLUDE_H
#define BEEB_HARDWARE_CRT_INCLUDE_H

#include "6845_crtc.h"
#include "5c094_vula.h"

class Crt {
public:
  Crt(const std::shared_ptr<Crtc>& crtc,
      const std::shared_ptr<VideoUla>& v_ula);

  void update_screen(const std::function<void(int32_t w, int32_t h, std::vector<uint8_t> scr_data)>& fn);
private:
  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<VideoUla> v_ula_;
  std::vector<uint8_t> screen_data_;
  int32_t pixel_x_;
  int32_t pixel_y_;

};

#endif //BEEB_HARDWARE_CRT_
