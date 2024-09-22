#ifndef APPS_BEEB_INCLUDE_LEDLABELWIDGET_H_
#define APPS_BEEB_INCLUDE_LEDLABELWIDGET_H_

#include <QLabel>

class LedLabelWidget : public QWidget {
 public:
  LedLabelWidget(const std::string& text, QWidget * parent = nullptr);

  bool is_on() const { return is_on_;}

 public slots:
  void turn_on();
  void turn_off();


 private:
  QPixmap led_off_pixmap_;
  QPixmap led_on_pixmap_;
  QLabel * text_;
  QLabel * led_;
  bool is_on_;
};
#endif // APPS_BEEB_INCLUDE_LEDLABELWIDGET_H_
