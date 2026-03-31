#include "execution_engine.h"

ExecutionEngine::ExecutionEngine(std::unique_ptr<Beeb> beeb)
    : beeb_{std::move(beeb)} {}

// ─── private helpers ────────────────────────────────────────────────────────

void ExecutionEngine::tick_one_instruction() {
  // Phase 1: drain the current opcode-fetch phase (SYNC is high while the
  //          6502 is fetching an opcode, so tick until it goes low).
  while (beeb_->bus()->tst_SYNC()) {
    beeb_->tick();
  }
  // Phase 2: run the instruction body until SYNC goes high again (next fetch).
  do {
    beeb_->tick();
  } while (!beeb_->bus()->tst_SYNC());
}

void ExecutionEngine::fire_callback() {
  if (!callback_) return;
  const auto& cpu = beeb_->cpu();
  auto pc = cpu->PC();
  uint32_t mem4 = 0;
  beeb_->get_memory_contents(pc, 4, reinterpret_cast<uint8_t*>(&mem4));
  callback_(pc, cpu->A(), cpu->X(), cpu->Y(), cpu->flags(), cpu->SP(), mem4);
}

// ─── public interface ────────────────────────────────────────────────────────

void ExecutionEngine::step_instruction() {
  tick_one_instruction();
  fire_callback();
  state_.store(PAUSED, std::memory_order_release);
}

void ExecutionEngine::run() {
  while (true) {
    tick_one_instruction();
    fire_callback();

    // Check for external pause (called from another thread).
    if (state_.load(std::memory_order_acquire) == PAUSED) break;

    // Check for step-out completion.
    if (state_.load(std::memory_order_acquire) == STEPPING_OUT) {
      if (beeb_->bus()->get_address() == step_out_target_) {
        state_.store(PAUSED, std::memory_order_release);
        break;
      }
    }

    // Check for breakpoint (fires when the NEXT instruction about to be
    // fetched matches a set address).
    if (is_breakpoint(beeb_->bus()->get_address())) {
      state_.store(PAUSED, std::memory_order_release);
      break;
    }
  }
}

void ExecutionEngine::step_out() {
  if (state_.load(std::memory_order_acquire) != PAUSED) return;

  // Read return address from the stack (JSR pushes PC-1 big-endian).
  auto sp = 0x100 | beeb_->cpu()->SP();
  auto lo = beeb_->memory()->data()->at(sp + 1);
  auto hi = beeb_->memory()->data()->at(sp + 2);
  step_out_target_ = static_cast<uint16_t>((hi * 256) + lo + 1);

  state_.store(STEPPING_OUT, std::memory_order_release);
}

void ExecutionEngine::pause() {
  int32_t expected = RUNNING;
  if (state_.compare_exchange_strong(expected, PAUSED,
                                     std::memory_order_acq_rel)) return;
  expected = STEPPING_OUT;
  state_.compare_exchange_strong(expected, PAUSED, std::memory_order_acq_rel);
}

void ExecutionEngine::resume() {
  int32_t expected = PAUSED;
  state_.compare_exchange_strong(expected, RUNNING, std::memory_order_acq_rel);
}

// ─── breakpoints ─────────────────────────────────────────────────────────────

void ExecutionEngine::add_breakpoint(uint16_t addr) {
  breakpoints_.insert(addr);
}

void ExecutionEngine::remove_breakpoint(uint16_t addr) {
  breakpoints_.erase(addr);
}

bool ExecutionEngine::is_breakpoint(uint16_t addr) const {
  return breakpoints_.count(addr) != 0;
}

const std::set<uint16_t>& ExecutionEngine::breakpoints() const {
  return breakpoints_;
}

// ─── callback ────────────────────────────────────────────────────────────────

void ExecutionEngine::set_instruction_callback(InstructionCallback cb) {
  callback_ = std::move(cb);
}

// ─── board / state accessors ─────────────────────────────────────────────────

Beeb& ExecutionEngine::board() {
  return *beeb_;
}

const Beeb& ExecutionEngine::board() const {
  return *beeb_;
}

bool ExecutionEngine::is_paused() const {
  return state_.load(std::memory_order_acquire) == PAUSED;
}
