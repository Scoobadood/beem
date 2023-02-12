#include "deeb_window.h"
#include "breakpoint_dlg.h"
#include "ui_deeb_window.h"
#include "ui_breakpoint_dlg.h"
#include "vdu_view.h"

#include <fstream>

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
  if (qApp->arguments().count() == 2) {
    mode = qApp->arguments().at(1).toInt() & 0x07;
  }
  beeb_ = new Beeb(mode);
  beeb_->set_pixel_function([=](std::vector<uint8_t> scr_data) {
    emit screen_changed(scr_data);
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

    QMetaObject::invokeMethod(
            this, [=]() { set_debug_buttons_paused(); }, Qt::QueuedConnection);
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


uint8_t map_key(Qt::Key key) {
  std::map<Qt::Key, uint8_t> keymap{
          {Qt::Key_A,                KEY_A},
          {Qt::Key_B,                KEY_B},
          {Qt::Key_C,                KEY_C},
          {Qt::Key_D,                KEY_D},
          {Qt::Key_E,                KEY_E},
          {Qt::Key_F,                KEY_F},
          {Qt::Key_G,                KEY_G},
          {Qt::Key_H,                KEY_H},
          {Qt::Key_I,                KEY_I},
          {Qt::Key_J,                KEY_J},
          {Qt::Key_K,                KEY_K},
          {Qt::Key_L,                KEY_L},
          {Qt::Key_M,                KEY_M},
          {Qt::Key_N,                KEY_N},
          {Qt::Key_O,                KEY_O},
          {Qt::Key_P,                KEY_P},
          {Qt::Key_Q,                KEY_Q},
          {Qt::Key_R,                KEY_R},
          {Qt::Key_S,                KEY_S},
          {Qt::Key_T,                KEY_T},
          {Qt::Key_U,                KEY_U},
          {Qt::Key_V,                KEY_V},
          {Qt::Key_W,                KEY_W},
          {Qt::Key_X,                KEY_X},
          {Qt::Key_Y,                KEY_Y},
          {Qt::Key_Z,                KEY_Z},
          {Qt::Key_0,                KEY_0},
          {Qt::Key_1,                KEY_1},
          {Qt::Key_2,                KEY_2},
          {Qt::Key_At,               KEY_2},
          {Qt::Key_3,                KEY_3},
          {Qt::Key_4,                KEY_4},
          {Qt::Key_5,                KEY_5},
          {Qt::Key_6,                KEY_6},
          {Qt::Key_7,                KEY_7},
          {Qt::Key_8,                KEY_8},
          {Qt::Key_9,                KEY_9},
          {Qt::Key_Space,            KEY_SPACE},
          {Qt::Key_Semicolon,        KEY_SEMI_COLON},
          {Qt::Key_Return,           KEY_RETURN},

          {Qt::Key_F10,              KEY_F0},
          {Qt::Key_F1,               KEY_F1},
          {Qt::Key_F2,               KEY_F2},
          {Qt::Key_F3,               KEY_F3},
          {Qt::Key_F4,               KEY_F4},
          {Qt::Key_F5,               KEY_F5},
          {Qt::Key_F6,               KEY_F6},
          {Qt::Key_F7,               KEY_F7},
          {Qt::Key_F8,               KEY_F8},
          {Qt::Key_F9,               KEY_F9},
          {Qt::Key_Minus,            KEY_MINUS},
          {Qt::Key_QuoteLeft,        KEY_CARET},
          {Qt::Key_AsciiTilde,       KEY_CARET},
          {Qt::Key_Escape,           KEY_ESC},
          {Qt::Key_Backslash,        KEY_BACK_SLASH},
          {Qt::Key_Left,             KEY_LT_ARROW},
          {Qt::Key_Right,            KEY_RT_ARROW},
          {Qt::Key_Tab,              KEY_TAB},
          {Qt::Key_BracketLeft,      KEY_LT_BRACE},
          {Qt::Key_Underscore,       KEY_UNDERSCORE},
          {Qt::Key_Up,               KEY_UP_ARROW},
          {Qt::Key_Down,             KEY_DN_ARROW},
          {Qt::Key_CapsLock,         KEY_CAPS_LOCK},
          {Qt::Key_Semicolon,        KEY_SEMI_COLON},
          {Qt::Key_Colon,            KEY_COLON},
          {Qt::Key_BracketRight,     KEY_RT_BRACE},
          {Qt::Key_Backspace,        KEY_DELETE},
          {Qt::Key_F12,              KEY_COPY},
          {Qt::Key_Tab,              KEY_TAB},
          {Qt::Key_Comma,            KEY_COMMA},
          {Qt::Key_Period,           KEY_PERIOD},
          {Qt::Key_Slash,            KEY_SLASH},
          {Qt::Key_ApplicationRight, KEY_SHIFT_LOCK},
          {Qt::Key_Shift,            KEY_SHIFT},
          {Qt::Key_Control,          KEY_CTL}
  };

  auto it = keymap.find(key);
  if (it != keymap.end()) {
    return it->second;
  };
  return 0x00;
}

void DeebWindow::keyPressEvent(QKeyEvent *event) {
  // Map key to scan code
  auto bbc_key = map_key((Qt::Key) event->key());
  if (bbc_key != 0) {
    beeb_->press_key(bbc_key);
  } else {
    spdlog::info("Untracked key : {}", event->key());
  }
}

void DeebWindow::keyReleaseEvent(QKeyEvent *event) {
  auto bbc_key = map_key((Qt::Key) event->key());
  if (bbc_key != 0) {
    beeb_->release_key(bbc_key);
  }
}

