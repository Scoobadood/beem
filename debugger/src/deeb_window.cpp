#include "deeb_window.h"
#include "ui_deeb_window.h"

#include <fstream>

#include <spdlog/spdlog-inl.h>
#include <QFileDialog>
#include <QThread>

DeebWindow::DeebWindow(QWidget *parent) //
    : QMainWindow(parent) //
    , ui(new Ui::DeebWindow) //
    , memory_{nullptr} //
{
  ui->setupUi(this);

  connect(ui->act_load_image, &QAction::triggered, this, &DeebWindow::load_file);
  connect(ui->act_load_rom, &QAction::triggered, this, &DeebWindow::load_rom);
  connect(ui->act_step, &QAction::triggered, this, &DeebWindow::step);


  std::shared_ptr<std::vector<uint8_t>> data =
      std::make_shared<std::vector<uint8_t>>(
          std::vector<uint8_t>{
              0xd8, 0xa2, 0xff, 0x9a, 0xa9, 0x00,
              0x8d, 0x00, 0x02, 0xa2, 0x05, 0x4c,
              0x33, 0x04, 0xa0, 0x05, 0xd0, 0x08,
              0x4c, 0x12, 0x04}
      );
  memory_ = new Memory(data->size());
  memory_->insert(0, *data);

  cpu_ = new M6502();
  ui->disasm_view->set_data(std::make_shared<std::vector<uint8_t>>(memory_->data()));
  ui->reg_view->set_cpu(cpu_);
  pins_ = cpu_->tick(pins_);
  set_RST(pins_);
}

DeebWindow::~DeebWindow() {
  delete ui;
}

/**
 * Step forward one instruction
 * - run til sync
 * - update flags and regs
 */
 void
 DeebWindow::step() {
  while (!tst_SYNC(pins_)) {
    pins_ = cpu_->tick(pins_);
    pins_ = memory_->tick(pins_);
  }
  ui->reg_view->update_flags();
 }


void
DeebWindow::load_file() {
  auto file_name = QFileDialog::getOpenFileName(this,
                                                tr("Load image"), "",
                                                tr("Image Files (*.bin);;All Files (*)"));
  if (file_name.isNull() || file_name.isEmpty()) {
    return;
  }

  auto fn = file_name.toStdString();
  std::ifstream file{fn, std::ios::in | std::ios::binary};
  if (file.fail()) {
    spdlog::error("Error reading file {}", fn);
    return;
  }

  memory_ = new Memory(file);
  file.close();
  ui->disasm_view->set_data(std::make_shared<std::vector<uint8_t>>(memory_->data()));
}

void
DeebWindow::load_rom() {
  auto file_name = QFileDialog::getOpenFileName(this,
                                                tr("Load ROM"), "",
                                                tr("Image Files (*.bin);;All Files (*)"));
  if (file_name.isNull() || file_name.isEmpty()) {
    return;
  }

  auto fn = file_name.toStdString();
  std::ifstream file{fn, std::ios::in | std::ios::binary};
  if (file.fail()) {
    spdlog::error("Error reading file {}", fn);
    return;
  }

  auto rom = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  memory_ = new Memory(65536);
  memory_->insert(0xc000, rom);
  ui->disasm_view->set_data(std::make_shared<std::vector<uint8_t>>(memory_->data()));
}
