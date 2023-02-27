#ifndef BEEB_HARDWARE_CRT_INCLUDE_H
#define BEEB_HARDWARE_CRT_INCLUDE_H

#include "6845_crtc.h"
#include "5c094_vula.h"

class Crt {
public:
  Crt(const std::shared_ptr<Crtc>& crtc,
      const std::shared_ptr<VideoUla>& v_ula);
private:
  std::shared_ptr<Crtc> crtc_;
  std::shared_ptr<VideoUla> v_ula_;
};

#endif //BEEB_HARDWARE_CRT_
