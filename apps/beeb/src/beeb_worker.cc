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

void BeebWorker::do_break() {
  reset_requested_.store(true, std::memory_order_release);
  engine_->pause();  // interrupt any in-progress run()
}

void BeebWorker::start_beeb() {
  engine_->board().reset();
  engine_->resume();

  while (!done_) {
    if (reset_requested_.exchange(false, std::memory_order_acq_rel)) {
      engine_->board().reset();
      engine_->resume();
      continue;
    }
    if (engine_->is_paused()) {
      QThread::msleep(10);
      continue;
    }
    engine_->run();
    emit_cpu_state();
    if (auto t = engine_->last_watch_trigger()) {
      auto [addr, old_val, new_val] = *t;
      emit watch_triggered(addr, old_val, new_val);
    }
    emit paused();
  }
  emit finished();
}

void BeebWorker::load_code(std::vector<uint8_t> code, uint16_t address) {
  /* TODO: this is hacky and not threadsafe. remove once tapeloader proper is done */
  engine_->board().load_data(code, address);
}

void BeebWorker::load_tape(std::shared_ptr<UefData> uef) {
  // Disconnect the old port before destroying it.
  engine_->board().set_cassette_port(nullptr);
  tape_stream_.reset();
  cassette_port_.reset();
  tape_data_.reset();

  tape_data_     = std::move(uef);
  tape_stream_   = std::make_unique<UefTapeStream>(*tape_data_);
  cassette_port_ = std::make_unique<CassettePort>();
  cassette_port_->set_stream(tape_stream_.get());
  engine_->board().set_cassette_port(cassette_port_.get());
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
