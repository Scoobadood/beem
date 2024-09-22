#ifndef BEEB_DEBUGGER_WINDOW_H_
#define BEEB_DEBUGGER_WINDOW_H_

#include <Disassembler/disassembler.h>
#include <QMainWindow>

#include "disassembly_view.h"
#include "register_view.h"
#include "breakpoint_manager.h"
#include "bus_view.h"

class DebuggerWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit DebuggerWindow(BreakpointManager * breakpoint_manager,
      QWidget *parent = nullptr);
  ~DebuggerWindow() override;

  DisassemblyView *view() { return disassembly_view_; }
  RegisterView *registers() { return register_view_; }

 public slots:
  void paused();
  void load_symbols();

 signals:
  void debugger_break();
  void debugger_run();
  void debugger_step();
  void debugger_step_out();

 private:
  void do_pause();
  void do_step();
  void do_step_out();
  void do_run();

  std::shared_ptr<Disassembler> disassembler_;
  DisassemblyView *disassembly_view_;
  RegisterView * register_view_;
  BusView * bus_view_;

  QAction *step_action_;
  QAction *step_out_action_;
  QAction *run_action_;
  QAction *pause_action_;
};
#endif // BEEB_DEBUGGER_WINDOW_H_
