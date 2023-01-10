#ifndef BUS_VIEW_H
#define BUS_VIEW_H

#include <QWidget>
#include "bus.h"

namespace Ui {
class BusView;
}

class BusView : public QWidget
{
    Q_OBJECT

public:
    explicit BusView(QWidget *parent = nullptr);
    ~BusView() override;

 public slots:
  void set_bus(const Bus & bus);

private:
    Ui::BusView *ui;

    uint16_t old_addr_;
    uint8_t old_data_;
};

#endif // BUS_VIEW_H
