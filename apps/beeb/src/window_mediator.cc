#include "window_mediator.h"
#include "breakpoint_dialog.h"
#include "history_window.h"
#include <QMenuBar>
#include <QFileDialog>

const QString SHOW_DEBUGGER = "Show &Debugger";
const QString HIDE_DEBUGGER = "Hide &Debugger";
const QString SHOW_MEMORY_INVESTIGATOR = "Show &Memory";
const QString HIDE_MEMORY_INVESTIGATOR = "Hide &Memory";
const QString SHOW_CASSETTE = "Show &Cassette";
const QString HIDE_CASSETTE = "Hide &Cassette";
const QString ENABLE_TRACING = "Enable tracing";
const QString DISABLE_TRACING = "Disable tracing";

WindowMediator::WindowMediator(
    BreakpointManager *breakpoint_manager,
    CrtWindow *main,
    DebuggerWindow *debugger_window,
    MemoryWindow *memory_window,
    CassetteWindow *cassette_window,
    BeebWorker *beeb_worker)
    : breakpoint_manager_{breakpoint_manager} //
    , main_(main) //
    , debugger_window_(debugger_window) //
    , memory_window_(memory_window) //
    , cassette_window_(cassette_window) //
    , beeb_worker_(beeb_worker) //
{
  toggle_debugger_action_ = new QAction(SHOW_DEBUGGER, this);
  toggle_memory_view_action_ = new QAction(SHOW_MEMORY_INVESTIGATOR, this);
  toggle_cassette_view_action_ = new QAction(SHOW_CASSETTE, this);

  // Debugger only
  load_symbols_action_     = new QAction("Load symbols", this);
  add_breakpoint_action_   = new QAction("Add Breakpoint", this);
  save_breakpoints_action_ = new QAction("Save Breakpoints...", this);
  load_breakpoints_action_ = new QAction("Load Breakpoints...", this);
  show_history_action_     = new QAction("Show History", this);
  load_uef_action_         = new QAction("Load UEF", this);
  toggle_trace_action_     = new QAction(ENABLE_TRACING, this);

  setup_main_window_menu();
  setup_debugger_window_menu();
  setup_memory_window_menu();
  setup_cassette_window_menu();

  assert(connect(load_symbols_action_,     &QAction::triggered, debugger_window_, &DebuggerWindow::load_symbols));
  assert(connect(add_breakpoint_action_,   &QAction::triggered, this, &WindowMediator::edit_breakpoints));
  assert(connect(save_breakpoints_action_, &QAction::triggered, this, &WindowMediator::save_breakpoints));
  assert(connect(load_breakpoints_action_, &QAction::triggered, this, &WindowMediator::load_breakpoints));
  assert(connect(show_history_action_,     &QAction::triggered, this, &WindowMediator::show_history));

  assert(connect(toggle_debugger_action_, &QAction::triggered, [this]() {
    if (toggle_debugger_action_->text() == SHOW_DEBUGGER) {
      show_debugger_view();
    } else {
      hide_debugger_view();
    }
  }));
  assert(connect(toggle_memory_view_action_, &QAction::triggered, [&]() {
    if (toggle_memory_view_action_->text() == SHOW_MEMORY_INVESTIGATOR) {
      show_memory_view();
    } else {
      hide_memory_view();
    }
  }));

  assert(connect(toggle_cassette_view_action_, &QAction::triggered, [&]() {
    if (toggle_cassette_view_action_->text() == SHOW_CASSETTE) {
      show_cassette_view();
    } else {
      hide_cassette_view();
    }
  }));

  assert(connect(toggle_trace_action_, &QAction::triggered, [&]() {
    if (toggle_trace_action_->text() == DISABLE_TRACING) {
      trace_off();
    } else {
      trace_on();
    }
  }));
  assert(connect(beeb_worker, &BeebWorker::trace, debugger_window_, &DebuggerWindow::trace));


      assert(connect(cassette_window_, &CassetteWindow::close_cassette_window, [&]() {
    hide_cassette_view();
  }));

  assert(connect(cassette_window_, &CassetteWindow::load_cassette_file,[&](const std::shared_ptr<TapeFile>& tf){
    beeb_worker_->load_code(tf->data, tf->load_addr);
  }));

  assert(connect(cassette_window_, &CassetteWindow::tape_inserted, [&](std::shared_ptr<UefData> uef) {
    beeb_worker_->load_tape(std::move(uef));
  }));
}

void WindowMediator::edit_breakpoints() {
  auto *dialog = new BreakpointDlg(breakpoint_manager_, debugger_window_);
  dialog->setWindowModality(Qt::ApplicationModal);
  dialog->show();
}

void WindowMediator::setup_debugger_window_menu() {
  auto view_menu = debugger_window_->menuBar()->addMenu(tr("&View"));
  view_menu->addAction(toggle_memory_view_action_);
  view_menu->addAction(toggle_cassette_view_action_);

  auto debugger_menu = debugger_window_->menuBar()->addMenu(tr("&Debugger"));
  debugger_menu->addAction(load_symbols_action_);
  debugger_menu->addAction(add_breakpoint_action_);
  debugger_menu->addAction(save_breakpoints_action_);
  debugger_menu->addAction(load_breakpoints_action_);
  debugger_menu->addAction(show_history_action_);
  debugger_menu->addAction(load_uef_action_);

  auto trace_menu = debugger_window_->menuBar()->addMenu(tr("&Trace"));
  trace_menu->addAction(toggle_trace_action_);
}

void WindowMediator::setup_main_window_menu() {
  auto view_menu = main_->menuBar()->addMenu(tr("&View"));
  view_menu->addAction(toggle_debugger_action_);
  view_menu->addAction(toggle_memory_view_action_);
  view_menu->addAction(toggle_cassette_view_action_);
}

void WindowMediator::setup_memory_window_menu() {
  auto view_menu = memory_window_->menuBar()->addMenu(tr("&View"));
  view_menu->addAction(toggle_debugger_action_);
  view_menu->addAction(toggle_cassette_view_action_);
}

void WindowMediator::setup_cassette_window_menu() {
  auto view_menu = cassette_window_->menuBar()->addMenu(tr("&View"));
  view_menu->addAction(toggle_debugger_action_);
  view_menu->addAction(toggle_memory_view_action_);
}

void WindowMediator::show_cassette_view() {
  toggle_cassette_view_action_->setText(HIDE_CASSETTE);
  cassette_window_->show();
}
void WindowMediator::hide_cassette_view() {
  toggle_cassette_view_action_->setText(SHOW_CASSETTE);
  cassette_window_->hide();
}
void WindowMediator::show_debugger_view() {
  toggle_debugger_action_->setText(HIDE_DEBUGGER);
  debugger_window_->show();
}
void WindowMediator::hide_debugger_view() {
  toggle_debugger_action_->setText(SHOW_DEBUGGER);
  debugger_window_->hide();
}
void WindowMediator::show_memory_view() {
  toggle_debugger_action_->setText(HIDE_MEMORY_INVESTIGATOR);
  memory_window_->show();
}
void WindowMediator::hide_memory_view() {
  toggle_debugger_action_->setText(SHOW_MEMORY_INVESTIGATOR);
  memory_window_->hide();
}
void WindowMediator::trace_on(){
  toggle_trace_action_->setText(DISABLE_TRACING);
  beeb_worker_->enable_tracing();
}
void WindowMediator::trace_off(){
  toggle_trace_action_->setText(ENABLE_TRACING);
  beeb_worker_->disable_tracing();
}

void WindowMediator::show_history() {
  auto history = beeb_worker_->engine().instruction_history();
  auto* win = new HistoryWindow(std::move(history),
                                debugger_window_->view()->disassembler(),
                                debugger_window_);
  win->setAttribute(Qt::WA_DeleteOnClose);
  win->show();
}

void WindowMediator::save_breakpoints() {
  auto path = QFileDialog::getSaveFileName(debugger_window_,
                                           "Save Breakpoints", "",
                                           "Breakpoint Files (*.bp);;All Files (*)");
  if (path.isNull() || path.isEmpty()) return;
  breakpoint_manager_->save_to_file(path.toStdString());
}

void WindowMediator::load_breakpoints() {
  auto path = QFileDialog::getOpenFileName(debugger_window_,
                                           "Load Breakpoints", "",
                                           "Breakpoint Files (*.bp);;All Files (*)");
  if (path.isNull() || path.isEmpty()) return;
  breakpoint_manager_->load_from_file(path.toStdString());
}


