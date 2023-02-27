#include "crt.h"

Crt::Crt(const std::shared_ptr<Crtc> &crtc,
         const std::shared_ptr<VideoUla> &v_ula) //
        : crtc_{crtc} //
        , v_ula_{v_ula} //
{}
