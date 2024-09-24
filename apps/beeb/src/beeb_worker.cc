#include "beeb_worker.h"

#include <QThread>
#include <QDebug>
#include <QCoreApplication>

#include <UEF/uef.h>

BeebWorker::BeebWorker(int32_t mode, BreakpointManager *breakpoint_manager)
    : branch_return_target_{0} //
    , breakpoint_manager_{breakpoint_manager} //
    , done_{false} //
    , state_{RUNNING} //
{
  beeb_ = std::make_shared<Beeb>(mode);
};

std::shared_ptr<Beeb> BeebWorker::beeb() {
  return beeb_;
}

void BeebWorker::pause() {
  state_.testAndSetRelaxed(RUNNING, PAUSED);
  state_.testAndSetRelaxed(STEPPING_OUT, PAUSED);
}

void BeebWorker::step() {
  state_.testAndSetRelaxed(PAUSED, STEPPING);
}

void BeebWorker::step_out() {
  if (state_.loadAcquire() == PAUSED) {
    // Pull target address from stack
    auto sp = 0x100 | beeb_->cpu()->SP();
    auto lo = beeb_->memory()->data()->at(sp + 1);
    auto hi = beeb_->memory()->data()->at(sp + 2);
    branch_return_target_ = (hi * 256) + lo + 1;
    // set state to stepping out
    state_.testAndSetRelaxed(PAUSED, STEPPING_OUT);
  }
}

void BeebWorker::run() {
  state_.testAndSetRelaxed(PAUSED, RUNNING);
}

void BeebWorker::start_beeb() {
  state_.storeRelaxed(RUNNING);
  beeb_->reset();

  // Run until done.
  while (!done_) {
    // If we're here, the main loop is not running and so we need to
    // check to see if we are still paused.
    if (state_.loadAcquire() == PAUSED) {
      QThread::msleep(10);
      continue;
    }

    // Not PAUSED so we're either running or stepping-out or stepping
    while (true) {
      // Step a single instruction
      while (beeb_->bus()->tst_SYNC()) {
        beeb_->tick();
      }
      do {
        beeb_->tick();
      } while (!beeb_->bus()->tst_SYNC());

      // Emit tracing data
      if (tracing_) {
        const auto &cpu = beeb_->cpu();
        auto pc = cpu->PC();
        // SYNC is high while fetching opcode so here we collect data from PC-1
        // We collect 4 bytes which is as much as we need for an opcode plus max 3 bytes data
        uint32_t memory_contents;
        beeb_->get_memory_contents(pc, 4, (uint8_t *)&memory_contents);
        emit trace(pc, cpu->A(), cpu->X(), cpu->Y(), cpu->flags(), cpu->SP(), memory_contents);
      }


      // If we're paused
      if (state_.loadAcquire() == PAUSED) {
        break;
      }

      // If we're stepping, we're paused now
      if (state_.testAndSetAcquire(STEPPING, PAUSED)) {
        break;
      }

      // If we're stepping-out, test whether PC is equal to address on top of stack
      auto state = state_.loadAcquire();
      if (state == STEPPING_OUT && (beeb_->bus()->get_address() == branch_return_target_)) {
        state_.storeRelaxed(PAUSED);
        break;
      }

      // We're stepping-out or running at this point so we need to check for a hit on
      // a breakpoint
      if (breakpoint_manager_->is_breakpoint(beeb_->bus()->get_address())) {
        state_.storeRelaxed(PAUSED);
        break;
      }
    }

    const auto &cpu = beeb_->cpu();
    emit flags_changed(cpu->flags());
    emit registers_changed(cpu->A(), cpu->X(), cpu->Y(), cpu->PC(), cpu->SP());
    emit pc_changed(cpu->PC());
    emit bus_changed(beeb_->bus());
    emit paused();
  }
  emit finished();
}

void BeebWorker::load_code(std::vector<uint8_t> code, uint16_t address) {
  /* TODO: this is hacky and not threadsafe. remover once tapeloader proper is done
   */
  beeb_->load_data(code, address);
}

void BeebWorker::enable_tracing() { tracing_ = true; }
void BeebWorker::disable_tracing() { tracing_ = false; }
