#include "debugger_window.h"
#include "spdlog/spdlog.h"

#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <fstream>
#include <QVBoxLayout>
#include <UEF/uef.h>

DebuggerWindow::DebuggerWindow(
    BreakpointManager * breakpoint_manager,
    QWidget *parent)
    : QMainWindow(parent) //
    , bus_view_{nullptr} //
    {
  setWindowTitle("Debugger");
  setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

  disassembler_ = std::make_shared<Disassembler>();
  disassembly_view_ = new DisassemblyView(breakpoint_manager, this);
  register_view_ = new RegisterView(this);

  auto tb = new QToolBar(this);

  step_action_ = new QAction("Step", this);
  assert(connect(step_action_, &QAction::triggered, this, &DebuggerWindow::do_step));
  tb->addAction(step_action_);

  step_out_action_ = new QAction("Step out", this);
  assert(connect(step_out_action_, &QAction::triggered, this, &DebuggerWindow::do_step_out));
  tb->addAction(step_out_action_);

  run_action_ = new QAction("Run", this);
  assert(connect(run_action_, &QAction::triggered, this, &DebuggerWindow::do_run));
  tb->addAction(run_action_);

  pause_action_ = new QAction("Pause", this);
  assert(connect(pause_action_, &QAction::triggered, this, &DebuggerWindow::do_pause));
  tb->addAction(pause_action_);

  addToolBar(tb);

  auto status_bar = new QStatusBar(this);
  status_bar->addWidget(register_view_);
  setStatusBar(status_bar);

  setCentralWidget(disassembly_view_);
}

DebuggerWindow::~DebuggerWindow() = default;

void
DebuggerWindow::do_step_out() {
  step_action_->setEnabled(false);
  step_out_action_->setEnabled(false);
  run_action_->setEnabled(false);
  pause_action_->setEnabled(true);
  emit debugger_step_out();
}

void
DebuggerWindow::do_step() {
  step_action_->setEnabled(false);
  step_out_action_->setEnabled(false);
  run_action_->setEnabled(false);
  pause_action_->setEnabled(true);
  emit debugger_step();
}

void
DebuggerWindow::do_run() {
  step_action_->setEnabled(false);
  step_out_action_->setEnabled(false);
  run_action_->setEnabled(false);
  pause_action_->setEnabled(true);
  emit debugger_run();
}

void DebuggerWindow::do_pause() {
  step_action_->setEnabled(true);
  step_out_action_->setEnabled(true);
  run_action_->setEnabled(true);
  pause_action_->setEnabled(false);
  emit debugger_break();
}

void DebuggerWindow::paused() {
  step_action_->setEnabled(true);
  step_out_action_->setEnabled(true);
  run_action_->setEnabled(true);
  pause_action_->setEnabled(false);
}

void DebuggerWindow::load_symbols() {
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
  disassembly_view_->set_symbols(symbols);
}
