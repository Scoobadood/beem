#include "acia_test_window.h"
#include "ui_acia_test_window.h"

AciaTestWindow::AciaTestWindow(const std::shared_ptr<DebuggableAcia> &acia, QWidget *parent)
    : QMainWindow(parent)//
    , ui(new Ui::AciaTestWindow)//
    , acia_{acia}//
{
  ui->setupUi(this);

  bus_ = std::make_shared<Bus>();
  assert(connect(ui->btn_tick, &QPushButton::clicked, this, &AciaTestWindow::tick));
  assert(connect(ui->btn_send_ctl, &QPushButton::clicked, this, &AciaTestWindow::send_ctl));
  assert(connect(ui->btn_send_data, &QPushButton::clicked, this, &AciaTestWindow::send_data));
  assert(connect(ui->btn_read_status, &QPushButton::clicked, this, &AciaTestWindow::read_status));

  assert(connect(ui->btn_set_cts, &QPushButton::clicked, this, &AciaTestWindow::set_cts));
  assert(connect(ui->btn_clr_cts, &QPushButton::clicked, this, &AciaTestWindow::clr_cts));

  reload_ui();
}


QString byte_value( uint8_t v) {
  return QString("%1").arg(v,2, 16, QChar('0'));
}

AciaTestWindow::~AciaTestWindow() {
  delete ui;
}

void
AciaTestWindow::tick() {
  acia_->tick(bus_);
  reload_ui();
  update();
}

void
AciaTestWindow::send_ctl() {
  bool ok = false;
  auto data = ui->le_ctl->text().toUInt(&ok, 16);
  if( ok ) {
    bus_->set_data(data);
    bus_->set_address(acia_->base_address());
    bus_->clr_RW();
    tick();
  } else {
    ui->le_ctl->clear();
  }
  bus_->set_address(0);
}

void
AciaTestWindow::read_status() {
  bus_->set_address(acia_->base_address());
  bus_->set_RW();
  tick();
  auto data = bus_->get_data();
  ui->lbl_status->setText(byte_value(data));
  bus_->set_address(0);
}

void
AciaTestWindow::send_data() {
  bool ok = false;
  auto data = ui->le_data->text().toUInt(&ok, 16);
  if( ok ) {
    bus_->set_data(data);
    bus_->set_address(acia_->base_address()+1);
    bus_->clr_RW();
    tick();
  } else {
    ui->le_ctl->clear();
  }
  bus_->set_address(0);
}

void AciaTestWindow::set_cts(){
  acia_->raise_cts();
}

void AciaTestWindow::clr_cts(){
  acia_->clear_cts();
}

void AciaTestWindow::reload_ui(){
  ui->le_sr->setText(byte_value(acia_->status_register()));
  ui->le_cr->setText(byte_value(acia_->control_register()));
  ui->le_tdr->setText(byte_value(acia_->tdr()));
  ui->le_rdr->setText(byte_value(acia_->rdr()));
  ui->le_tx_shift->setText(byte_value(acia_->tx_shift_register()));
  ui->le_rx_shift->setText(byte_value(acia_->rx_shift_register()));

  ui->lbl_cts_pin->setText(QString::number(acia_->cts() ? 1 : 0));
}