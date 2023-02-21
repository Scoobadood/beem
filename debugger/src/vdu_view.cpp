//
// Created by Dave Durbin on 31/1/2023.
//

#include "vdu_view.h"
#include "keyboard.h"
#include "key_mapper.h"
#include "beeb.h"
#include "string_to_keystrokes.h"

#include <QWidget>
#include <QLabel>
#include <QKeyEvent>

#include <spdlog/spdlog-inl.h>
#include <QCoreApplication>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <utility>

VduView::VduView(QWidget *parent) //
        : QLabel{parent} //
{
  setScaledContents(false);
  image_ = new QImage(SCR_WIDTH, SCR_HEIGHT, QImage::Format_RGB888);
  pixmap_ = new QPixmap();
  setAutoFillBackground(true);
  setStyleSheet("background-color:#00a0a0");
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setAlignment(Qt::AlignCenter);
  setAttribute(Qt::WA_Hover);
}

void VduView::screen_changed(std::vector<uint8_t> scr_data) {
  // SHIT Qt support for images
  for (int y = 0; y < SCR_HEIGHT; y++) {
    memcpy(image_->scanLine(y), scr_data.data() + (y * SCR_WIDTH * 3), SCR_WIDTH * 3);
  }
  // Make a scaled copy
  setUpdatesEnabled(false);
  pixmap_->convertFromImage(*image_);
  int w = contentsRect().width();
  int h = contentsRect().height();
  setPixmap(*pixmap_);
//  if (w < h) {
//    int ah = (w * 576.0 / 720.0);
//    setPixmap(pixmap_->scaled(w, ah, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
//  } else {
//    int aw = (h * 720.0 / 576.0);
//    setPixmap(pixmap_->scaled(aw, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
//  }
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

void VduView::paste_data(const QString &text) {

  class MyThread : public QThread {
  public:
    MyThread(std::shared_ptr<Beeb> beeb, QString text, QObject *parent = nullptr) //
            : QThread(parent)//
            , text_{std::move(text)} //
            , beeb_{std::move(beeb)}//
    {}

  protected:
    QString text_;
    std::shared_ptr<Beeb> beeb_;

    void run() override {
      auto buff = string_to_keystrokes(text_.toStdString());
      for (auto i=0; i<buff.size(); ++i ) {
        beeb_->press_key(buff[i]);
        if( buff[i] == KEY_SHIFT)
          beeb_->press_key(buff[++i]);
        msleep(50);
        if( buff[i] == KEY_RETURN) {
          msleep(500);
        }
        beeb_->release_key(buff[i]);
        if( buff[i-1] == KEY_SHIFT)
          beeb_->release_key(buff[i-1]);
        msleep(50);
      }
    }
  };

  (new MyThread(beeb_, text))->start();
}
