#ifndef BEEB_INCLUDE_WINDOW_MEDIATOR_H_
#define BEEB_INCLUDE_WINDOW_MEDIATOR_H_

#include <QObject>
#include "breakpoint_manager.h"
#include "crt_window.h"
#include "debugger_window.h"
#include "memory_window.h"

class WindowMediator : public QObject {
 Q_OBJECT

 public:
  WindowMediator(BreakpointManager * breakpoint_manager, CrtWindow *main, DebuggerWindow *debugger_window, MemoryWindow *memory_window);

 public slots:
  void edit_breakpoints();

 private:
  void setup_debugger_menu();
  void setup_main_menu();
  void setup_memory_menu();
  BreakpointManager * breakpoint_manager_;
  CrtWindow *main_;
  DebuggerWindow *debugger_window_;
  MemoryWindow *memory_window_;

  QAction * toggle_debugger_action_;
  QAction * toggle_memory_view_action_;
  QAction * load_symbols_action_;
  QAction * add_breakpoint_action_;
};

#endif // BEEB_INCLUDE_WINDOW_MEDIATOR_H_
