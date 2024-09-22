#ifndef BEEB_WORKER_H_
#define BEEB_WORKER_H_

#include <QObject>
#include <QAtomicInteger>
#include "beeb.h"
#include "breakpoint_manager.h"

class BeebWorker : public QObject {
 Q_OBJECT

 public:
  BeebWorker(int32_t mode,
             BreakpointManager *breakpoint_manager);
  std::shared_ptr<Beeb> beeb();

 public slots :
  void start_beeb();
  void pause();
  void step();
  void step_out();
  void run();

 signals:
  void finished();
  void paused();
  void flags_changed(uint8_t);
  void registers_changed(uint8_t a, uint8_t x, uint8_t y, uint16_t pc, uint8_t sp);
  void pc_changed(uint16_t pc);
  void bus_changed(std::shared_ptr<Bus> bus);

 private:
  const int32_t PAUSED = 0;
  const int32_t STEPPING = 1;
  const int32_t STEPPING_OUT = 2;
  const int32_t RUNNING = 3;

  uint16_t branch_return_target_;

  std::shared_ptr<Beeb> beeb_;
  BreakpointManager *breakpoint_manager_;
  bool done_;

  QAtomicInteger<int32_t> state_;
};

#endif // BEEB_WORKER_H_
