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
  image_ = new QImage(640, 224, QImage::Format_RGB888);
  pixmap_ = new QPixmap();
  setAutoFillBackground(true);
  setStyleSheet("background-color:#00a0a0");
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setAlignment(Qt::AlignCenter);
}

void VduView::screen_changed(std::vector<uint8_t> scr_data) {
  // SHIT Qt support for images
  for (int y = 0; y < 224; y++) {
    memcpy(image_->scanLine(y), scr_data.data()+(y * 640 * 3), 640 * 3);
  }
  // Make a scaled copy
  setUpdatesEnabled(false);
  pixmap_->convertFromImage(*image_);
  int w = contentsRect().width();
  int h = contentsRect().height();
  if (w < h) {
    int ah = (w * 576.0 / 720.0);
    setPixmap(pixmap_->scaled(w, ah, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
  } else {
    int aw = (h * 720.0 / 576.0);
    setPixmap(pixmap_->scaled(aw, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
  }
  setUpdatesEnabled(true);
}
