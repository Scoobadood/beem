#include "window_mediator.h"
#include "breakpoint_dialog.h"
#include <QMenuBar>

const QString SHOW_DEBUGGER = "Show &Debugger";
const QString HIDE_DEBUGGER = "Hide &Debugger";
const QString SHOW_MEMORY_INVESTIGATOR = "Show &Memory";
const QString HIDE_MEMORY_INVESTIGATOR = "Hide &Memory";
const QString SHOW_CASSETTE = "Show &Cassette";
const QString HIDE_CASSETTE = "Hide &Cassette";

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
  load_symbols_action_ = new QAction("Load symbols", this);
  add_breakpoint_action_ = new QAction("Add Breakpoint", this);
  load_uef_action_ = new QAction("Load UEF", this);

  setup_main_window_menu();
  setup_debugger_window_menu();
  setup_memory_window_menu();
  setup_cassette_window_menu();

  assert(connect(load_symbols_action_, &QAction::triggered, debugger_window_, &DebuggerWindow::load_symbols));
  assert(connect(add_breakpoint_action_, &QAction::triggered, this, &WindowMediator::edit_breakpoints));

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

  assert(connect(cassette_window_, &CassetteWindow::close_cassette_window, [&]() {
    hide_cassette_view();
  }));

  assert(connect(cassette_window_, &CassetteWindow::load_cassette_file,[&](const std::shared_ptr<TapeFile>& tf){
    beeb_worker_->load_code(tf->data, tf->load_addr);
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
  debugger_menu->addAction(load_uef_action_);
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
