#include "deeb_window.h"
#include "breakpoint_dlg.h"
#include "ui_deeb_window.h"
#include "ui_breakpoint_dlg.h"

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

  connect(ui->act_load_symbols, &QAction::triggered, this, &DeebWindow::load_symbols);

  ui->act_step->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  ui->act_run->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

  beeb_ = new Beeb();

  // Merge RAM and ROM
  auto beeb_memory = std::make_shared<std::vector<uint8_t>>();
  const auto & ram =  beeb_->memory()->data();
  const auto & rom = beeb_->mos()->data();
  beeb_memory->insert(beeb_memory->end(), ram->begin(), ram->end());
  beeb_memory->insert(beeb_memory->end(), 16384, 0);
  beeb_memory->insert(beeb_memory->end(), rom.begin(), rom.end());
  ui->disasm_view->set_data(beeb_memory, 0);
  ui->mem_view->set_memory(beeb_memory);
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
  } while (!beeb_->bus()->tst_SYNC());
  const auto &cpu = beeb_->cpu();
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
    if (beeb_->bus()->tst_SYNC() && (breakpoints_.count(beeb_->bus()->get_address()) != 0)) break;
  } while (true);
  const auto &cpu = beeb_->cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
  emit bus_changed(beeb_->bus());

  ui->act_run->setDisabled(false);
  ui->act_step->setDisabled(false);

}

void DeebWindow::load_symbols() {
  auto file_name = QFileDialog::getOpenFileName(this,
                                                tr("Load Symbols"), "",
                                                tr("Symbol Files (*.txt);;All Files (*)"));
  if (file_name.isNull() || file_name.isEmpty()) {
    return;
  }
  std::ifstream file{file_name.toStdString(), std::ios::in};
  if (file.fail()) {
    spdlog::error("Error reading file {} ", file_name.toStdString());
  }

  std::string line;
  std::map<uint16_t, std::string> symbols;
  while (getline(file, line)) {
    // Split into address and symbol
    auto idx = line.find(' ');
    if (idx != -1) {
      auto addr_txt = line.substr(0, idx);
      auto symbol = line.substr(idx);
      auto addr = (uint16_t) std::stoi(addr_txt, nullptr, 16);
      symbols.emplace(addr, symbol);
    }
  }
  file.close();

  // Force repeat disassembly
  ui->disasm_view->set_symbols(symbols);
}

void DeebWindow::breakpoint_set(uint16_t brk_addr) {
  breakpoints_.emplace(brk_addr);
}

void DeebWindow::breakpoint_cleared(uint16_t brk_addr) {
  breakpoints_.erase(brk_addr);
}

void DeebWindow::on_act_edit_breakpoints_triggered() {
  auto *d = new BreakpointDlg();
  d->set_breakpoints(breakpoints_);
  auto btn = d->exec();
  auto brks = d->breakpoints();
  for (auto b: brks) {
    if (breakpoints_.count(b) > 0) continue;
    breakpoint_set(b);
  }
  d->deleteLater();
}

