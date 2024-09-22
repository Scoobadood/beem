#include "window_mediator.h"
#include "breakpoint_dialog.h"
#include <QMenuBar>

const QString SHOW_DEBUGGER = "Show &Debugger";
const QString HIDE_DEBUGGER = "Hide &Debugger";
const QString SHOW_MEMORY_INVESTIGATOR = "Show &Memory";
const QString HIDE_MEMORY_INVESTIGATOR = "Hide &Memory";

WindowMediator::WindowMediator(
    BreakpointManager *breakpoint_manager,
    CrtWindow *main, DebuggerWindow *debugger_window, MemoryWindow *memory_window)
    : breakpoint_manager_{breakpoint_manager} //
    , main_(main) //
    , debugger_window_(debugger_window) //
    , memory_window_(memory_window) //
{
  toggle_debugger_action_ = new QAction(SHOW_DEBUGGER, this);
  toggle_memory_view_action_ = new QAction(SHOW_MEMORY_INVESTIGATOR, this);

  // Debugger only
  load_symbols_action_ = new QAction("Load symbols", this);
  add_breakpoint_action_ = new QAction("Add Breakpoint", this);

  setup_main_menu();
  setup_debugger_menu();
  setup_memory_menu();

  assert(connect(load_symbols_action_, &QAction::triggered, debugger_window_, &DebuggerWindow::load_symbols));
  assert(connect(add_breakpoint_action_, &QAction::triggered, this, &WindowMediator::edit_breakpoints));
  assert(connect(toggle_debugger_action_, &QAction::triggered, [this]() {
    if (toggle_debugger_action_->text() == SHOW_DEBUGGER) {
      toggle_debugger_action_->setText(HIDE_DEBUGGER);
      debugger_window_->show();
    } else {
      toggle_debugger_action_->setText(SHOW_DEBUGGER);
      debugger_window_->hide();
    }
  }));
  assert(connect(toggle_memory_view_action_, &QAction::triggered, [&]() {
    if (toggle_memory_view_action_->text() == SHOW_MEMORY_INVESTIGATOR) {
      toggle_memory_view_action_->setText(HIDE_MEMORY_INVESTIGATOR);
      memory_window_->show();
    } else {
      toggle_memory_view_action_->setText(SHOW_MEMORY_INVESTIGATOR);
      memory_window_->hide();
    }
  }));
}

void WindowMediator::edit_breakpoints() {
  auto *dialog = new BreakpointDlg(breakpoint_manager_, debugger_window_);
  dialog->setWindowModality(Qt::ApplicationModal);
  dialog->show();
}

void WindowMediator::setup_debugger_menu() {
  auto debugger_menu = debugger_window_->menuBar()->addMenu(tr("&Debugger"));
  debugger_menu->addAction(load_symbols_action_);
  debugger_menu->addAction(toggle_memory_view_action_);
  debugger_menu->addAction(add_breakpoint_action_);
}

void WindowMediator::setup_main_menu() {
  auto view_menu = main_->menuBar()->addMenu(tr("&View"));
  view_menu->addAction(toggle_debugger_action_);
  view_menu->addAction(toggle_memory_view_action_);
}

void WindowMediator::setup_memory_menu() {}
