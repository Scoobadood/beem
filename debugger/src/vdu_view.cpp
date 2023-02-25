//
// Created by Dave Durbin on 31/1/2023.
//

#include "vdu_view.h"
#include "keyboard.h"
#include "key_mapper.h"
#include "beeb.h"

#include <QWidget>
#include <QLabel>
#include <QKeyEvent>

#include <QCoreApplication>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>

VduView::VduView(QWidget *parent) //
        : QLabel{parent} //
{
  setScaledContents(false);
  last_width_ = SCR_WIDTH;
  last_height_ = SCR_HEIGHT;
  image_ = new QImage(SCR_WIDTH, SCR_HEIGHT, QImage::Format_RGB888);
  pixmap_ = new QPixmap();
  setAutoFillBackground(true);
  setStyleSheet("background-color:#00a0a0");
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setAlignment(Qt::AlignCenter);
  setAttribute(Qt::WA_Hover);
}

void VduView::screen_changed(int32_t width, int32_t height, const std::vector<uint8_t>& scr_data) {
  // ARBITRARY values to prevent ugliness while machine is booting.
  if( width < 1 || width > 1024 || height < 1 || height > 1024) return;
  if (width != last_width_ || height != last_height_) {
    delete image_;
    image_ = new QImage(width, height, QImage::Format_RGB888);
  }
  for (int y = 0; y < height; y++) {
    memcpy(image_->scanLine(y), scr_data.data() + (y * width * 3), width * 3);
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
  last_height_ = height;
  last_width_ = width;
  setUpdatesEnabled(true);
}

void VduView::keyPressEvent(QKeyEvent *event) {
  uint8_t bbc_key;
  bool shift_pressed;

  if (!map_key_combination(event->keyCombination(), bbc_key, shift_pressed)) {
    spdlog::info("Untracked key : {}", event->key());
    return;
  }

  if (shift_pressed)
    beeb_->press_key(KEY_SHIFT);
  else
    beeb_->release_key(KEY_SHIFT);
  beeb_->press_key(bbc_key);
}

void VduView::keyReleaseEvent(QKeyEvent *event) {
  uint8_t bbc_key;
  bool shift_pressed;

  if (!map_key_combination(event->keyCombination(), bbc_key, shift_pressed)) {
    return;
  }

  beeb_->release_key(bbc_key);
  if ((event->modifiers() & Qt::KeyboardModifier::ShiftModifier) == 0) {
    beeb_->release_key(KEY_SHIFT);
  }
}

void VduView::enterEvent(QEnterEvent *event) {
  // Install key grabber
  qApp->installEventFilter(this);
  QLabel::enterEvent(event);
}

void VduView::leaveEvent(QEvent *event) {
  qApp->removeEventFilter(this);
  QLabel::leaveEvent(event);
}

bool VduView::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
    keyPressEvent(keyEvent);
    return true;
  } else if (event->type() == QEvent::KeyRelease) {
    auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
    keyReleaseEvent(keyEvent);
    return true;
  }
  return false;
}
