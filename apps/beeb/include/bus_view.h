#ifndef BUS_VIEW_H
#define BUS_VIEW_H

#include <QWidget>
#include <QRadioButton>
#include <QLabel>

#include "bus.h"

class BusView : public QWidget {
 Q_OBJECT

 public:
  explicit BusView(QWidget *parent = nullptr);
  ~BusView() override;

 public slots:
  void set_bus(const std::shared_ptr<Bus> &bus);

 private:
  uint16_t old_addr_;
  uint8_t old_data_;

  QRadioButton *rb_reset_;
  QRadioButton *rb_sync_;
  QRadioButton *rb_rw_;
  QLabel * lbl_addr_bin_hi_;
  QLabel * lbl_addr_bin_lo_;
  QLabel * lbl_addr_hex_;
  QLabel * lbl_data_bin_;
  QLabel * lbl_data_hex_;
};

#endif // BUS_VIEW_H
