#ifndef BEEB_WORKER_H_
#define BEEB_WORKER_H_

#include <QObject>
#include "beeb.h"
#include "execution_engine.h"
#include "cassette_port.h"
#include "uef_tape_stream.h"
#include <UEF/uef.h>
#include <atomic>
#include <memory>
#include <string>

class BeebWorker : public QObject {
 Q_OBJECT

 public:
  explicit BeebWorker(int32_t mode);

  // Access the underlying board (replaces beeb()).
  Beeb& board();

  void load_code(std::vector<uint8_t> code, uint16_t address);
  void load_tape(std::shared_ptr<UefData> uef);
  void enable_tracing();
  void disable_tracing();

  // Expose engine so callers (e.g. main.cc) can wire breakpoints to it.
  ExecutionEngine& engine();

 public slots :
  void start_beeb();
  void pause();
  void step();
  void step_out();
  void run();
  void do_break();

 signals:
  void finished();
  void paused();
  void flags_changed(uint8_t);
  void registers_changed(uint8_t a, uint8_t x, uint8_t y, uint16_t pc, uint8_t sp);
  void pc_changed(uint16_t pc);
  void bus_changed(std::shared_ptr<Bus> bus);
  void trace(uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t flags, uint16_t sp, uint32_t data);
  // Emitted when execution paused because a watched address changed value.
  void watch_triggered(uint16_t addr, uint8_t old_val, uint8_t new_val);

 private:
  void emit_cpu_state();

  std::unique_ptr<ExecutionEngine> engine_;
  bool done_;
  std::atomic<bool> reset_requested_{false};

  // Currently loaded tape — owned here, plugged into Beeb via set_cassette_port().
  std::unique_ptr<CassettePort>  cassette_port_;
  std::unique_ptr<UefTapeStream> tape_stream_;
  std::shared_ptr<UefData>       tape_data_;
};

#endif // BEEB_WORKER_H_
