#ifndef BEEB_VDU_VIEW_H
#define BEEB_VDU_VIEW_H

#include <QWidget>

class VduView : public QWidget {
 Q_OBJECT

 public:
  explicit VduView(QWidget *parent = nullptr);

  void screen_changed(int32_t width, int32_t height, const std::vector<uint8_t> &scr_data);

  void keyPressEvent(QKeyEvent *event) override;

  void keyReleaseEvent(QKeyEvent *event) override;

  void enterEvent(QEnterEvent *event) override;

  void leaveEvent(QEvent *event) override;

  bool eventFilter(QObject *object, QEvent *event) override;

//  void set_beeb(const std::shared_ptr<Beeb> &beeb) {beeb_ = beeb;};

 signals:
  void press_key(uint8_t key);
  void release_key(uint8_t key);

 protected:
  void paintEvent(QPaintEvent *event) override;

 private:
  int32_t last_width_;
  int32_t last_height_;
  QImage *image_;
};

#endif //BEEB_VDU_VIEW_H
