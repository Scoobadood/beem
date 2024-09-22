#include "vdu_view.h"
#include "keyboard.h"
#include "spdlog/spdlog.h"
#include "key_mapper.h"

#include <QWidget>
#include <QKeyEvent>

#include <QCoreApplication>
#include <QApplication>
#include <QPainter>

// Display Region of the CRT
const int32_t DE_LEFT = 176;
const int32_t DE_TOP = 61;
const int32_t DE_WIDTH = 640;
const int32_t DE_HEIGHT = 512;

VduView::VduView(QWidget *parent) //
    : QWidget{parent} //
    , last_width_{-1} //
    , last_height_{-1} //
    , image_{nullptr} //
{
}

void VduView::screen_changed(int32_t width, int32_t height, const std::vector<uint8_t> &scr_data) {
  if (width != last_width_ || height != last_height_) {
    delete image_;
    image_ = new QImage(scr_data.data(), width, height, QImage::Format_RGB888);
    last_width_ = width;
    last_height_ = height;
  } else {
    memcpy(image_->bits(), scr_data.data(), width * height * 3);
  }
  update();
}

void VduView::paintEvent(QPaintEvent *event) {
  if (!image_) return;
  QPainter painter(this);

  // Source rect - eiher the whole ting or just the display area
  QRect sourceRect(DE_LEFT, DE_TOP, DE_WIDTH, DE_HEIGHT);
//  QRect sourceRect(0, 0, image_->width(), image_->height());

  double aspectRatio = static_cast<double>(DE_WIDTH) / DE_HEIGHT;
  auto widgetWidth = width();
  auto widgetHeight = height();

  int32_t targetWidth, targetHeight;
  if (widgetWidth / static_cast<double>(widgetHeight) > aspectRatio) {
    targetHeight = widgetHeight;
    targetWidth = static_cast<int>(targetHeight * aspectRatio);
  } else {
    targetWidth = widgetWidth;
    targetHeight = static_cast<int>(targetWidth / aspectRatio);
  }

  // Center the rectangle within the widget
  QRect targetRect;
  targetRect.setLeft((widgetWidth - targetWidth) / 2);
  targetRect.setTop((widgetHeight - targetHeight) / 2);
  targetRect.setWidth(targetWidth);
  targetRect.setHeight(targetHeight);

  painter.drawImage(targetRect, *image_, sourceRect);
}

void VduView::keyPressEvent(QKeyEvent *event) {
  uint8_t bbc_key;
  bool shift_pressed;

  if (!map_key_combination(event->keyCombination(), bbc_key, shift_pressed)) {
    spdlog::warn("Untracked key : {}", event->key());
    return;
  }

  if (shift_pressed)
      emit press_key(KEY_SHIFT);
  else
      emit release_key(KEY_SHIFT);
  emit press_key(bbc_key);
}

void VduView::keyReleaseEvent(QKeyEvent *event) {
  uint8_t bbc_key;
  bool shift_pressed;

  if (!map_key_combination(event->keyCombination(), bbc_key, shift_pressed)) {
    return;
  }

  emit release_key(bbc_key);
  if ((event->modifiers() & Qt::KeyboardModifier::ShiftModifier) == 0) {
    emit release_key(KEY_SHIFT);
  }
}

void VduView::enterEvent(QEnterEvent *event) {
  // Install key grabber
  qApp->installEventFilter(this);
  QWidget::enterEvent(event);
}

void VduView::leaveEvent(QEvent *event) {
  qApp->removeEventFilter(this);
  QWidget::leaveEvent(event);
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
