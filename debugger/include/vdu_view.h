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

  void screen_changed(std::vector<uint8_t> scr_data);

  void keyPressEvent(QKeyEvent *event) override;

  void keyReleaseEvent(QKeyEvent *event) override;

  void enterEvent(QEnterEvent *event) override;

  void leaveEvent(QEvent *event) override;

  bool eventFilter(QObject *object, QEvent *event) override;

  void set_beeb(const std::shared_ptr<Beeb> &beeb) {beeb_ = beeb;};

private:
  QPixmap *pixmap_;
  QImage *image_;
  std::shared_ptr<Beeb> beeb_;
};


#endif //BEEB_VDU_VIEW_H
