#include "beeb_worker.h"

#include <QThread>
#include <QDebug>
#include <QCoreApplication>

BeebWorker::BeebWorker(int32_t mode)
    : done_{false} //
{
  engine_ = std::make_unique<ExecutionEngine>(std::make_unique<Beeb>(mode));
}

Beeb& BeebWorker::board() {
  return engine_->board();
}

ExecutionEngine& BeebWorker::engine() {
  return *engine_;
}

// ─── slots ────────────────────────────────────────────────────────────────────

void BeebWorker::pause() {
  engine_->pause();
}

void BeebWorker::step() {
  engine_->step_instruction();
  emit_cpu_state();
  emit paused();
}

void BeebWorker::step_out() {
  engine_->step_out();
}

void BeebWorker::run() {
  engine_->resume();
}

void BeebWorker::start_beeb() {
  engine_->board().reset();
  engine_->resume();

  while (!done_) {
    if (engine_->is_paused()) {
      QThread::msleep(10);
      continue;
    }
    engine_->run();
    emit_cpu_state();
    emit paused();
  }
  emit finished();
}

void BeebWorker::load_code(std::vector<uint8_t> code, uint16_t address) {
  /* TODO: this is hacky and not threadsafe. remove once tapeloader proper is done */
  engine_->board().load_data(code, address);
}

void BeebWorker::enable_tracing() {
  engine_->set_instruction_callback(
      [this](uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
             uint8_t flags, uint8_t sp, uint32_t mem4) {
        emit trace(pc, a, x, y, flags, sp, mem4);
      });
}

void BeebWorker::disable_tracing() {
  engine_->set_instruction_callback(nullptr);
}

// ─── private helpers ──────────────────────────────────────────────────────────

void BeebWorker::emit_cpu_state() {
  const auto& cpu = engine_->board().cpu();
  emit flags_changed(cpu->flags());
  emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
  emit pc_changed(cpu->PC());
  emit bus_changed(engine_->board().bus());
}
