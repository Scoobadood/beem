//
// Created by Dave Durbin on 31/1/2023.
//

#ifndef BEEB_VDU_VIEW_H
#define BEEB_VDU_VIEW_H

#include <QLabel>
#include "beeb.h"

class VduView : public QLabel {
Q_OBJECT

public:
  explicit VduView(QWidget *parent = nullptr);

  void screen_changed(int32_t width, int32_t height, const std::vector<uint8_t> & scr_data);

  void keyPressEvent(QKeyEvent *event) override;

  void keyReleaseEvent(QKeyEvent *event) override;

  void enterEvent(QEnterEvent *event) override;

  void leaveEvent(QEvent *event) override;

  bool eventFilter(QObject *object, QEvent *event) override;

  void set_beeb(const std::shared_ptr<Beeb> &beeb) {beeb_ = beeb;};

private:
  int32_t last_width_;
  int32_t last_height_;
  QPixmap *pixmap_;
  QImage *image_;
  std::shared_ptr<Beeb> beeb_;
};


#endif //BEEB_VDU_VIEW_H
