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
{
  ui->setupUi(this);

  connect(ui->act_step, &QAction::triggered, this, &DeebWindow::step);
  connect(ui->act_run, &QAction::triggered, this, &DeebWindow::run);

  connect(this, &DeebWindow::flags_changed, ui->reg_view, &RegisterView::set_flags);
  connect(this, &DeebWindow::registers_changed, ui->reg_view, &RegisterView::set_registers);
  connect(this, &DeebWindow::pc_changed, ui->disasm_view, &DisassemblyView::set_current_address);
  connect(this, &DeebWindow::bus_changed, ui->bus_view, &BusView::set_bus);

  connect(ui->disasm_view, &DisassemblyView::breakpoint_set, this, &DeebWindow::breakpoint_set);
  connect(ui->disasm_view, &DisassemblyView::breakpoint_cleared, this, &DeebWindow::breakpoint_cleared);

  ui->act_step->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  ui->act_run->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

  beeb_ = new Beeb();
  ui->disasm_view->set_data(std::make_shared<std::vector<uint8_t>>(beeb_->memory()->data()));
  reset_cpu();
}

DeebWindow::~DeebWindow() {
  delete ui;
}

/**
 * Toggle RST line and wait for CPU to reset.
 */

void DeebWindow::reset_cpu() {
  beeb_->reset();
  auto cpu = beeb_->cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
}

/**
 * Step forward one instruction
 * - run til sync
 * - update flags and regs
 */
void
DeebWindow::step() {
  ui->act_step->setDisabled(true);

  do {
    beeb_->tick();
  } while (!beeb_->bus().tst_SYNC());
  const auto & cpu = beeb_->cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
  emit bus_changed(beeb_->bus());

  ui->act_step->setDisabled(false);
}

/**
 * Step forward one instruction
 * - run til sync
 * - update flags and regs
 */
void
DeebWindow::run() {
  ui->act_run->setDisabled(true);
  ui->act_step->setDisabled(true);

  do {
    beeb_->tick();
    if (beeb_->bus().tst_SYNC() && (breakpoints_.count(beeb_->bus().get_address()) != 0)) break;
  } while (true);
  const auto & cpu = beeb_->cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
  emit bus_changed(beeb_->bus());

  ui->act_run->setDisabled(false);
  ui->act_step->setDisabled(false);

}

void DeebWindow::breakpoint_set(uint16_t brk_addr) {
  breakpoints_.emplace(brk_addr);
}

void DeebWindow::breakpoint_cleared(uint16_t brk_addr) {
  breakpoints_.erase(brk_addr);
}
