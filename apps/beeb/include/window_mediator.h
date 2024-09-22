#ifndef BEEB_INCLUDE_WINDOW_MEDIATOR_H_
#define BEEB_INCLUDE_WINDOW_MEDIATOR_H_

#include <QObject>
#include "breakpoint_manager.h"
#include "crt_window.h"
#include "debugger_window.h"
#include "memory_window.h"
#include "beeb_worker.h"
#include "cassette_window.h"

class WindowMediator : public QObject {
 Q_OBJECT

 public:
  WindowMediator(BreakpointManager * breakpoint_manager,
                 CrtWindow *main,
                 DebuggerWindow *debugger_window,
                 MemoryWindow *memory_window,
                 CassetteWindow * cassette_window,
                 BeebWorker * beeb_worker);

  void show_cassette_view();
  void hide_cassette_view();
  void show_debugger_view();
  void hide_debugger_view();
  void show_memory_view();
  void hide_memory_view();

 public slots:
  void edit_breakpoints();

 private:
  void setup_debugger_window_menu();
  void setup_main_window_menu();
  void setup_memory_window_menu();
  void setup_cassette_window_menu();

  BreakpointManager * breakpoint_manager_;
  CrtWindow *main_;
  DebuggerWindow *debugger_window_;
  MemoryWindow *memory_window_;
  CassetteWindow *cassette_window_;
  BeebWorker * beeb_worker_;

  QAction * toggle_debugger_action_;
  QAction * toggle_memory_view_action_;
  QAction * toggle_cassette_view_action_;
  QAction * load_symbols_action_;
  QAction * add_breakpoint_action_;
  QAction * load_uef_action_;
};

#endif // BEEB_INCLUDE_WINDOW_MEDIATOR_H_
