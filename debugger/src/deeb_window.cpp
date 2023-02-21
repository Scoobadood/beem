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



QString crtc_test="REM Intro to CRTC Registers by Kieran Connell.\n"
                      "REM CRTC Register explorer.\n"
                      "REM Use cursor keys to change register values.\n"
                      "REM Press R to reset to MODE 1 defaults.\n"
                      "MODE 1\n"
                      "VDU 19,0,4;0;\n"
                      "*FX4,1\n"
                      "ON ERROR REPORT:PRINT;\" at line \";ERL:END\n"
                      "FOR Y%=0 TO 31:PRINT TAB(0,Y%);Y%;:NEXT\n"
                      "FOR X%=0 TO 39:PRINT TAB(X%,0);X%MOD10;:NEXT\n"
                      "PRINT TAB(10,21);\"Use cursor keys to\";TAB(10,22);\"change register values\";TAB(10,23);\"Press R to reset\"\n"
                      "M%=11:DIM REGS(M%):DIM NAME$(M%)\n"
                      "PROCresetregs\n"
                      "A%=0\n"
                      "REPEAT\n"
                      "PRINT TAB(9,17);\"Total scanlines = \";(REGS(4)+1)*(REGS(9)+1)+REGS(5)\n"
                      "PRINT TAB(11,18);\"Displayed RAM = \";(REGS(1)*8*REGS(6))/1024;\"K    \"\n"
                      "PRINT TAB(4,4+A%);\"=>\";\n"
                      "K=GET\n"
                      "PRINT TAB(4,4+A%);\"  \";\n"
                      "IF K=139 AND A%>0 THEN A%=A%-1\n"
                      "IF K=138 AND A%<M% THEN A%=A%+1\n"
                      "IF K=136 THEN PROCsetreg(A%,REGS(A%)-1)\n"
                      "IF K=137 THEN PROCsetreg(A%,REGS(A%)+1)\n"
                      "IF K=ASC(\"R\") THEN PROCresetregs\n"
                      "UNTIL FALSE\n"
                      "END\n"
                      "DEF PROCresetregs\n"
                      "RESTORE\n"
                      "FOR I%=0 TO M%:READ NAME$(I%),REGS(I%):PROCsetreg(I%, REGS(I%)):NEXT\n"
                      "ENDPROC\n"
                      "DEF PROCprintreg(R%)\n"
                      "PRINT TAB(7,4+R%);NAME$(R%);TAB(29,4+R%);\"R\";R%;\"=\";REGS(R%);\"  \"\n"
                      "ENDPROC\n"
                      "DEF PROCsetreg(R%, V%)\n"
                      "REGS(R%)=V%\n"
                      "?&FE00=R%:?&FE01=V%\n"
                      "PROCprintreg(R%)\n"
                      "ENDPROC\n"
                      "REM CRTC Register defaults for MODE 1.\n"
                      "DATA \"Horizontal total -1\", 127  : REM R0\n"
                      "DATA \"Horizontal displayed\", 80  : REM R1\n"
                      "DATA \"Horizontal sync pos\", 98   : REM R2\n"
                      "DATA \"Horizontal sync width\", &28: REM R3\n"
                      "DATA \"Vertical total -1\", 38     : REM R4       \n"
                      "DATA \"Vertical total adjust\", 0  : REM R5\n"
                      "DATA \"Vertical displayed\", 32    : REM R6\n"
                      "DATA \"Vertical sync pos\", 35     : REM R7\n"
                      "DATA \"Interlace control\", 0      : REM R8\n"
                      "DATA \"Scanlines per row -1\", 7   : REM R9\n"
                      "DATA \"Cursor start\", &67         : REM R10\n"
                      "DATA \"Cursor end\", 8             : REM R11";

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

  connect(ui->act_load_rom, &QAction::triggered, [&](){ui->crt_view->paste_data(crtc_test);});

  connect(this, &DeebWindow::screen_changed, ui->crt_view, &VduView::screen_changed);

  ui->act_step->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  ui->act_run->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));


  uint8_t mode = 7;
  if (qApp->arguments().count() == 2) {
    mode = qApp->arguments().at(1).toInt() & 0x07;
  }
  beeb_ = std::make_shared<Beeb>(mode);
  beeb_->set_pixel_function([=](const std::vector<uint8_t> & scr_data) {
    emit screen_changed(scr_data);
  });

  ui->crt_view->set_beeb(beeb_);
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