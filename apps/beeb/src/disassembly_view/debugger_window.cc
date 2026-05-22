#include "debugger_window.h"

#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <fstream>
#include <QVBoxLayout>
#include <UEF/uef.h>
#include <Disassembler/symbol_file_loader.h>
#include <Disassembler/operation_formatter.h>

const uint32_t MAX_TRACE_BUFFER_SIZE = 1'000'000;

DebuggerWindow::DebuggerWindow(
    BreakpointManager *breakpoint_manager,
    QWidget *parent)
    : QMainWindow(parent) //
    , bus_view_{nullptr} //
    , watch_label_{nullptr} //
    , trace_buffer_{} //
    , trace_buffer_idx_{0}//
{
  setWindowTitle("Debugger");
  setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

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

  watch_label_ = new QLabel(this);
  watch_label_->setStyleSheet("color: darkorange; font-family: Monaco; font-size: 11px;");

  auto status_bar = new QStatusBar(this);
  status_bar->addWidget(register_view_);
  status_bar->addPermanentWidget(watch_label_);
  setStatusBar(status_bar);

  setCentralWidget(disassembly_view_);

  trace_buffer_.reserve(MAX_TRACE_BUFFER_SIZE);
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

void DebuggerWindow::watch_triggered(uint16_t addr, uint8_t old_val, uint8_t new_val) {
  auto msg = QString("Watch &%1: $%2 → $%3")
      .arg(addr, 4, 16, QChar('0'))
      .arg(old_val, 2, 16, QChar('0'))
      .arg(new_val, 2, 16, QChar('0'));
  watch_label_->setText(msg);
}

void DebuggerWindow::load_symbols() {
  auto file_name = QFileDialog::getOpenFileName(this,
                                                tr("Load Symbols"), "",
                                                tr("Symbol Files (*.txt);;All Files (*)"));
  if (file_name.isNull() || file_name.isEmpty()) {
    return;
  }

  auto symbols = load_symbols_from_file(file_name.toStdString());

  disassembly_view_->disassembler().set_symbols(symbols);
  // Force redraw
  update();
}

void DebuggerWindow::push_to_trace_buffer(const std::string& item) {
  if( trace_buffer_idx_ < MAX_TRACE_BUFFER_SIZE ) {
    trace_buffer_.push_back(item);
  } else {
    trace_buffer_.at(trace_buffer_idx_) = item;
  }
  trace_buffer_idx_ = (trace_buffer_idx_+1) % MAX_TRACE_BUFFER_SIZE;
}

void DebuggerWindow::trace(uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t flags, uint16_t sp, uint32_t memory) {
  uint8_t err;
  uint16_t offset = 0;
  auto op = disassembly_view_->disassembler().disassemble_one((const uint8_t *) &memory, 4u, offset, err);
  // Make new op with correct address/label
  auto &symbols = disassembly_view_->disassembler().symbols();
  auto iter = symbols.find(pc);
  auto label = (iter == symbols.end())
               ? ""
               : iter->second.name;

  std::string line;
  if( label.size() > 20) {
    push_to_trace_buffer(label);
    std::cout << label << std::endl;
    auto op2 = Operation{"", pc, op.opcode, op.data};
    line = format_single_line(op2, disassembly_view_->disassembler().symbols());
  } else {
    auto op2 = Operation{label, pc, op.opcode, op.data};
    line = format_single_line(op2, disassembly_view_->disassembler().symbols());
  }

  line = fmt::format("{:50s} A:{:02x} X:{:02x} Y:{:02x} F:{}",
                    line, a, x, y, format_flags(flags)
  );
  push_to_trace_buffer(line);
  std::cout << line << std::endl;
}