#include "deeb_window.h"
#include "ui_deeb_window.h"

#include <fstream>

#include <spdlog/spdlog-inl.h>
#include <QFileDialog>
#include <QThread>
#include <QStyle>

DeebWindow::DeebWindow(QWidget *parent) //
    : QMainWindow(parent) //
    , ui(new Ui::DeebWindow) //
    , memory_{nullptr} //
{
  ui->setupUi(this);

  connect(ui->act_load_image, &QAction::triggered, this, &DeebWindow::load_file);
  connect(ui->act_load_rom, &QAction::triggered, this, &DeebWindow::load_rom);
  connect(ui->act_step, &QAction::triggered, this, &DeebWindow::step);
  connect(this, &DeebWindow::flags_changed, ui->reg_view, &RegisterView::set_flags);
  connect(this, &DeebWindow::registers_changed, ui->reg_view, &RegisterView::set_registers);
  connect(this, &DeebWindow::pc_changed, ui->disasm_view, &DisassemblyView::set_current_address);

  ui->act_step->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

  cpu_ = new M6502();
}

DeebWindow::~DeebWindow() {
  delete ui;
}

/**
 * Toggle RST line and wait for CPU to reset.
 */

void DeebWindow::reset_cpu() {
  clr_RST(pins_);
  pins_ = cpu_->tick(pins_);
  set_RST(pins_);
  do {
    pins_ = cpu_->tick(pins_);
    pins_ = memory_->tick(pins_);
  } while (!tst_SYNC(pins_));
  emit flags_changed(cpu_->flags());
  emit registers_changed(cpu_->A(), cpu_->X(), cpu_->Y(), cpu_->PC(), cpu_->SP());
  emit pc_changed(cpu_->PC());
}

/**
 * Step forward one instruction
 * - run til sync
 * - update flags and regs
 */
void
DeebWindow::step() {
  do {
    pins_ = cpu_->tick(pins_);
    pins_ = memory_->tick(pins_);
  } while (!tst_SYNC(pins_));
  emit flags_changed(cpu_->flags());
  emit registers_changed(cpu_->A(), cpu_->X(), cpu_->Y(), cpu_->PC(), cpu_->SP());
  emit pc_changed(cpu_->PC());
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
  reset_cpu();
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
  reset_cpu();
}
