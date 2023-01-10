#include "bus_view.h"
#include "ui_bus_view.h"

BusView::BusView(QWidget *parent)
    : QWidget(parent) //
    , ui(new Ui::BusView) //
    , old_addr_{0} //
    , old_data_{0} //
{
  ui->setupUi(this);
}

void set_binary_value(QLabel *label, uint8_t value) {
  label->setText(QString("%1").arg(value, 8, 2, QChar('0')));
}

void BusView::set_bus(const Bus &bus) {
  auto new_addr = bus.get_address();
  if ((new_addr & 0xff00) != (old_addr_ & 0xff00)) {
    set_binary_value(ui->lbl_addr_bin_hi, new_addr >> 8);
    ui->lbl_addr_bin_hi->setStyleSheet("QLabel {color:red; }");
  } else {
    ui->lbl_addr_bin_hi->setStyleSheet("QLabel {color:black; }");
  }

  if ((new_addr & 0xff) != (old_addr_ & 0xff)) {
    set_binary_value(ui->lbl_addr_bin_lo, new_addr & 0xff);
    ui->lbl_addr_bin_lo->setStyleSheet("QLabel {color:red; }");
  } else {
    ui->lbl_addr_bin_lo->setStyleSheet("QLabel {color:black; }");
  }
  old_addr_ = new_addr;
  ui->lbl_addr_hex->setText(QString("%1").arg(new_addr, 4, 16, QChar('0')));

  auto new_data = bus.get_data();
  if (old_data_ != new_data) {
    set_binary_value(ui->lbl_data_bin, new_data);
    ui->lbl_data_bin->setStyleSheet("QLabel {color:red; }");
  } else {
    ui->lbl_data_bin->setStyleSheet("QLabel {color:black; }");
  }
  old_data_ = new_data;
  ui->lbl_data_hex->setText(QString("%1").arg(new_data, 2, 16, QChar('0')));

  ui->rb_sync->setChecked(bus.tst_SYNC());
  ui->rb_reset->setChecked(bus.tst_RST());
  ui->rb_rw->setChecked(bus.tst_RW());
}

BusView::~BusView() {
  delete ui;
}
