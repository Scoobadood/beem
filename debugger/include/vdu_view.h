//
// Created by Dave Durbin on 31/1/2023.
//

#ifndef BEEB_VDU_VIEW_H
#define BEEB_VDU_VIEW_H

#include <QLabel>

class VduView : public QLabel {
Q_OBJECT

public:
  explicit VduView(QWidget *parent = nullptr);

  void screen_changed(std::vector<uint8_t> scr_data);


private:
  QPixmap *pixmap_;
  QImage *image_;
};


#endif //BEEB_VDU_VIEW_H
