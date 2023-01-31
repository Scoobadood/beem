#include "deeb_window.h"
#include "breakpoint_dlg.h"
#include "ui_deeb_window.h"
#include "ui_breakpoint_dlg.h"
#include "vdu_view.h"

#include <fstream>

#include <spdlog/spdlog-inl.h>
#include <QFileDialog>
#include <QThread>
#include <QStyle>
#include <QtConcurrent/QtConcurrent>
#include <QKeyEvent>

DeebWindow::DeebWindow(QWidget *parent) //
        : QMainWindow(parent) //
        , ui(new Ui::DeebWindow) //
        , brk_requested_{false} //
{
  ui->setupUi(this);

  connect(ui->act_step, &QAction::triggered, this, &DeebWindow::step);
  connect(ui->act_run, &QAction::triggered, this, &DeebWindow::run);
  connect(ui->act_break, &QAction::triggered, this, &DeebWindow::brk);
  connect(ui->act_reset, &QAction::triggered, this, &DeebWindow::reset_cpu);

  connect(this, &DeebWindow::flags_changed, ui->reg_view, &RegisterView::set_flags);
  connect(this, &DeebWindow::registers_changed, ui->reg_view, &RegisterView::set_registers);
  connect(this, &DeebWindow::pc_changed, ui->disassembly_view, &DisassemblyView::set_pc);

  connect(ui->disassembly_view, &DisassemblyView::breakpoint_set, this, &DeebWindow::breakpoint_set);
  connect(ui->disassembly_view, &DisassemblyView::breakpoint_cleared, this, &DeebWindow::breakpoint_cleared);
  connect(ui->mem_view, &MemoryView::needs_data, this, &DeebWindow::beeb_data_needed);
  connect(ui->disassembly_view, &DisassemblyView::needs_data, this, &DeebWindow::beeb_data_needed);

  connect(ui->act_load_symbols, &QAction::triggered, this, &DeebWindow::load_symbols);

  connect(this, &DeebWindow::screen_changed, ui->crt_view, &VduView::screen_changed);

  ui->act_step->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  ui->act_run->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

  uint8_t mode = 7;
  if(qApp->arguments().count() == 2 ) {
    mode = qApp->arguments().at(1).toInt() & 0x07;
  }
  beeb_ = new Beeb(mode);
  beeb_->set_pixel_function([=](uint8_t *scr_data, uint32_t sz) {
    emit screen_changed(scr_data, sz);
  });

  reset_cpu();
}

DeebWindow::~DeebWindow() {
  delete ui;
}


void DeebWindow::set_debug_buttons_paused() {
  ui->act_break->setEnabled(false);
  ui->act_run->setEnabled(true);
  ui->act_step->setEnabled(true);
  ui->act_reset->setEnabled(true);
}

void DeebWindow::set_debug_buttons_running() {
  ui->act_break->setEnabled(true);
  ui->act_run->setEnabled(false);
  ui->act_step->setEnabled(false);
  ui->act_reset->setEnabled(false);
}

void DeebWindow::beeb_data_needed(QWidget *source, uint16_t start_address, uint32_t num_bytes) {
  auto data = beeb_->get_memory_contents(start_address, num_bytes);
  if (source == ui->mem_view) {
    ui->mem_view->set_data(data);
  } else if (source == ui->disassembly_view) {
    ui->disassembly_view->set_data(data);
  }
}

/**
 * Toggle RST line and wait for CPU to reset.
 */

void DeebWindow::reset_cpu() {
  set_debug_buttons_running();
  beeb_->reset();
  auto cpu = beeb_->cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
  set_debug_buttons_paused();
}

/**
 * Step forward one instruction
 * - run til sync
 * - update flags and regs
 */
void
DeebWindow::step() {
  set_debug_buttons_running();
  // Wait for sync to clear
  while (beeb_->bus()->tst_SYNC()) {
    beeb_->tick();
  }
  // Now wait for it to be set.
  do {
    beeb_->tick();
  } while (!beeb_->bus()->tst_SYNC());
  const auto &cpu = beeb_->cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
  emit bus_changed(beeb_->bus());

  set_debug_buttons_paused();
}

/**
 * Step forward one instruction
 * - run til sync
 * - update flags and regs
 */
void
DeebWindow::run() {
  set_debug_buttons_running();

  brk_requested_ = false;
  QtConcurrent::run([&] {
    do {
      beeb_->tick();
      if (beeb_->bus()->tst_SYNC() && (breakpoints_.count(beeb_->bus()->get_address()) != 0)) break;
    } while (!brk_requested_);
    const auto &cpu = beeb_->cpu();
    emit flags_changed(cpu->flags());
    emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
    emit pc_changed(cpu->PC());
    emit bus_changed(beeb_->bus());

    set_debug_buttons_paused();
  });
}

void DeebWindow::brk() {
  brk_requested_ = true;
  ui->act_break->setEnabled(false);
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
  ui->disassembly_view->set_symbols(symbols);
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
  ui->disassembly_view->update_breakpoints(breakpoints_);
  d->deleteLater();
}

void DeebWindow::keyPressEvent(QKeyEvent *event) {
  spdlog::info( "DeebWindow saw keypress");
  // Map key to scan code
  if( event->key() == Qt::Key_M) beeb_->press_key(KEY_M);
  if( event->key() == Qt::Key_O) beeb_->press_key(KEY_O);
  if( event->key() == Qt::Key_D) beeb_->press_key(KEY_D);
  if( event->key() == Qt::Key_E) beeb_->press_key(KEY_E);
  if( event->key() == Qt::Key_Space) beeb_->press_key(KEY_SPACE);
  if( event->key() == Qt::Key_4) beeb_->press_key(KEY_4);
  if( event->key() == Qt::Key_0) beeb_->press_key(KEY_0);
  if( event->key() == Qt::Key_Return) beeb_->press_key(KEY_RETURN);
}
void DeebWindow::keyReleaseEvent(QKeyEvent *event) {
  if( event->key() == Qt::Key_M) beeb_->release_key(KEY_M);
  if( event->key() == Qt::Key_O) beeb_->release_key(KEY_O);
  if( event->key() == Qt::Key_D) beeb_->release_key(KEY_D);
  if( event->key() == Qt::Key_E) beeb_->release_key(KEY_E);
  if( event->key() == Qt::Key_Space) beeb_->release_key(KEY_SPACE);
  if( event->key() == Qt::Key_4) beeb_->release_key(KEY_4);
  if( event->key() == Qt::Key_0) beeb_->release_key(KEY_0);
  if( event->key() == Qt::Key_Return) beeb_->release_key(KEY_RETURN);
}

