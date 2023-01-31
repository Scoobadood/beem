//
// Created by Dave Durbin on 31/1/2023.
//

#include "vdu_view.h"

#include <QWidget>
#include <QLabel>

#include <spdlog/spdlog-inl.h>

VduView::VduView(QWidget *parent) //
        : QLabel{parent} //
{
  setScaledContents(false);
  image_ = new QImage(640,224, QImage::Format_RGB888);
  pixmap_ = new QPixmap();
  setAutoFillBackground(true);
  setStyleSheet("background-color:#00a0a0");
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setAlignment(Qt::AlignCenter);
}

void VduView::screen_changed(uint8_t *scr_data, uint32_t sz) {
  // SHIT Qt support for images
  for (int y = 0; y < 224; y++) {
    memcpy(image_->scanLine(y), &scr_data[y * 640 * 3], y * 640 * 3);
  }
  spdlog::info( "memcpy completed");

  // Make a scaled copy
  setUpdatesEnabled(false);
  spdlog::info( "updates disabled");
  pixmap_->convertFromImage(*image_);
  spdlog::info( "pixmap converted from image");
//
  spdlog::info( "got contentsRect width");
  int w = contentsRect().width();
  spdlog::info( "got contentsRect height");
  int h = contentsRect().height();
  if( w < h) {
    int ah = (w * 576.0 / 720.0);
    spdlog::info( "Calculated ah");
    setPixmap(pixmap_->scaled(w, ah, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
  } else {
    int aw = (h * 720.0 / 576.0);
    spdlog::info( "Calculated aw");
    setPixmap(pixmap_->scaled(aw, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
  }
  setUpdatesEnabled(true);
  spdlog::info( "Re-enabled updates");
}
