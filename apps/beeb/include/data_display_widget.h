#ifndef BEEB_INCLUDE_DATA_DISPLAY_WIDGET_H_
#define BEEB_INCLUDE_DATA_DISPLAY_WIDGET_H_

#include <QWidget>

class DataDisplayWidget : public QWidget {
 public:
  DataDisplayWidget(QWidget *parent = nullptr) :
      QWidget(parent) {}
  virtual void set_data(const std::vector<uint8_t> & data) = 0;
};

#endif // BEEB_INCLUDE_DATA_DISPLAY_WIDGET_H_
