#include "crt.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <iostream>

const int32_t SCR_BUFF_WIDTH = 1024;
const int32_t SCR_BUFF_HEIGHT = 626;

Crt::Crt(const std::shared_ptr<Crtc> &crtc,
         const std::shared_ptr<VideoUla> &v_ula) //
    : crtc_{crtc} //
    , v_ula_{v_ula} //
    , renderer_{nullptr} //
    , pixel_x_{0} //
    , pixel_y_{0} //
    , last_vs_{false} //
    , last_hs_{false} //
    , fps_frame_count_{0} //
    , fps_last_time_{std::chrono::steady_clock::now()} //
{
  screen_data_.resize(SCR_BUFF_WIDTH * SCR_BUFF_HEIGHT * 3, 0);
  even_frame_ = true;

  try {
    auto logger = spdlog::basic_logger_mt("CRT", "logs/CRT.txt", true);
    logger->flush_on(spdlog::level::err);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
  logger_ = spdlog::get("CRT");
}

void
Crt::tick() {

  auto vs = crtc_->vsync();
  auto hs = crtc_->hsync();

  if (!hs && last_hs_) {
    pixel_x_ = 0;
    pixel_y_ += 2;
  }

  if (!vs && last_vs_) {
    pixel_x_ = even_frame_ ? 0 : pixel_x_;
    pixel_y_ = even_frame_ ? 1 : 0;
    even_frame_ = !even_frame_;
    renderer_(SCR_BUFF_WIDTH, SCR_BUFF_HEIGHT, screen_data_);

    ++fps_frame_count_;
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - fps_last_time_).count();
    if (elapsed_ms >= 1000) {
      double fps = fps_frame_count_ * 1000.0 / elapsed_ms;
      fprintf(stdout, "FPS: %.1f\n", fps);
      fflush(stdout);
      fps_frame_count_ = 0;
      fps_last_time_ = now;
    }
  }

  if (pixel_x_ < SCR_BUFF_WIDTH && pixel_y_ < SCR_BUFF_HEIGHT) {
    uint32_t pixel_colour;
    if (crtc_->display_enable()) {
      pixel_colour = v_ula_->rgb();
    } else {
      pixel_colour = 0x808080 | (vs ? 0xff0000u : 0u) | (hs ? 0x00ff00u : 0u);
    }
    auto pixel_addr_ = ((pixel_y_ * SCR_BUFF_WIDTH) + pixel_x_) * 3;
    uint8_t* p = screen_data_.data() + pixel_addr_;
    p[0] = (pixel_colour >> 16) & 0xff;
    p[1] = (pixel_colour >> 8) & 0xff;
    p[2] =  pixel_colour & 0xff;
  }

  last_hs_ = hs;
  last_vs_ = vs;
  ++pixel_x_;
}