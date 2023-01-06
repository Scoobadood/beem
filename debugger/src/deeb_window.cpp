#include "../include/deeb_window.h"
#include "./ui_deeb_window.h"

DeebWindow::DeebWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::DeebWindow) {
  ui->setupUi(this);

  std::shared_ptr<std::vector<uint8_t>> data =
      std::make_shared<std::vector<uint8_t>>(
      std::vector<uint8_t>{
      0xd8, 0xa2, 0xff, 0x9a, 0xa9, 0x00,
      0x8d, 0x00, 0x02, 0xa2, 0x05, 0x4c,
      0x33, 0x04, 0xa0, 0x05, 0xd0, 0x08,
      0x4c, 0x12, 0x04}
      );
  ui->disasm_view->set_data(data);
}

DeebWindow::~DeebWindow() {
  delete ui;
}

