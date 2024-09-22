

#include <spdlog/cfg/env.h>
#include <QApplication>
#include <QThread>

#include "crt_window.h"
#include "beeb_worker.h"
#include "window_mediator.h"

QThread *init_worker(const std::unique_ptr<BeebWorker> &beeb_worker) {
  auto worker_thread = new QThread();  // Create a separate thread
  beeb_worker->moveToThread(worker_thread);
  assert(QObject::connect(worker_thread, &QThread::started, beeb_worker.get(), &BeebWorker::start_beeb));
  assert(QObject::connect(beeb_worker.get(), &BeebWorker::finished, worker_thread, &QThread::quit));
  assert(QObject::connect(beeb_worker.get(), &BeebWorker::finished, beeb_worker.get(), &BeebWorker::deleteLater));
  assert(QObject::connect(worker_thread, &QThread::finished, worker_thread, &QThread::deleteLater));
  return worker_thread;
}

void config_debugger_window(const std::unique_ptr<BeebWorker> &beeb_worker,
                            CrtWindow *main_window,
                            DebuggerWindow *debugger_window) {
  assert(QObject::connect(debugger_window->view(),
                          &DisassemblyView::needs_data,
                          main_window,
                          &CrtWindow::data_requested));

  assert(QObject::connect(debugger_window, &DebuggerWindow::debugger_break,
                          beeb_worker.get(), &BeebWorker::pause,
                          Qt::DirectConnection));
  assert(QObject::connect(debugger_window, &DebuggerWindow::debugger_step,
                          beeb_worker.get(), &BeebWorker::step,
                          Qt::DirectConnection));
  assert(QObject::connect(debugger_window, &DebuggerWindow::debugger_step_out,
                          beeb_worker.get(), &BeebWorker::step_out,
                          Qt::DirectConnection));
  assert(QObject::connect(debugger_window, &DebuggerWindow::debugger_run,
                          beeb_worker.get(), &BeebWorker::run,
                          Qt::DirectConnection));

  assert(QObject::connect(beeb_worker.get(), &BeebWorker::paused,
                          debugger_window, &DebuggerWindow::paused));

  assert(QObject::connect(beeb_worker.get(), &BeebWorker::pc_changed,
                          debugger_window->view(), &DisassemblyView::set_pc));

  assert(QObject::connect(beeb_worker.get(), &BeebWorker::flags_changed,
                          debugger_window->registers(), &RegisterView::set_flags));

  assert(QObject::connect(beeb_worker.get(), &BeebWorker::registers_changed,
                          debugger_window->registers(), &RegisterView::set_registers));
}

void config_memory_window(CrtWindow *main_window,
                          MemoryWindow *memory_window) {
  assert(QObject::connect(memory_window->view(),
                          &MemoryView::needs_data,
                          main_window,
                          &CrtWindow::data_requested));
}

int main(int argc, char *argv[]) {
  spdlog::cfg::load_env_levels();
  QApplication a(argc, argv);

  auto breakpoint_manager = new BreakpointManager();
  auto beeb_worker = std::make_unique<BeebWorker>(1, breakpoint_manager);
  auto worker_thread = init_worker(beeb_worker);

  // Memory window
  auto memory_window = new MemoryWindow();

  // Debugger window
  auto debugger_window = new DebuggerWindow(breakpoint_manager);

  // Main window
  auto crt_window = new CrtWindow(beeb_worker->beeb());

  config_memory_window(crt_window, memory_window);
  config_debugger_window(beeb_worker, crt_window, debugger_window);

  auto window_mediator = new WindowMediator(breakpoint_manager, crt_window, debugger_window, memory_window);
  crt_window->show();
  worker_thread->start();

  auto return_code = QApplication::exec();

  // Flushes logs
  spdlog::shutdown();

  delete crt_window;
  delete debugger_window;
  delete memory_window;
  delete worker_thread;
  return return_code;
}
