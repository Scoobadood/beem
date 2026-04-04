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
  const auto& cpu = beeb_->cpu();
  const auto pc   = cpu->PC();
  uint32_t mem4   = 0;
  beeb_->get_memory_contents(pc, 4, reinterpret_cast<uint8_t*>(&mem4));

  // Always record to ring buffer
  history_buf_[history_head_] = {pc, cpu->A(), cpu->X(), cpu->Y(),
                                  cpu->SP(), cpu->flags(), mem4};
  history_head_ = (history_head_ + 1) % HISTORY_CAPACITY;
  if (history_count_ < HISTORY_CAPACITY) ++history_count_;

  if (callback_) {
    callback_(pc, cpu->A(), cpu->X(), cpu->Y(), cpu->flags(), cpu->SP(), mem4);
  }
}

// ─── public interface ────────────────────────────────────────────────────────

void ExecutionEngine::step_instruction() {
  tick_one_instruction();
  fire_callback();
  state_.store(PAUSED, std::memory_order_release);
}

void ExecutionEngine::run() {
  last_watch_trigger_.reset();

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

    // Check watch breakpoints.
    auto triggered = check_watches();
    if (triggered) {
      last_watch_trigger_ = triggered;
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

// ─── watches ─────────────────────────────────────────────────────────────────

void ExecutionEngine::add_watch(uint16_t addr, std::optional<uint8_t> trigger_value) {
  uint8_t current{0};
  beeb_->get_memory_contents(addr, 1, &current);
  watches_[addr] = {current, trigger_value};
}

void ExecutionEngine::remove_watch(uint16_t addr) {
  watches_.erase(addr);
}

bool ExecutionEngine::is_watch(uint16_t addr) const {
  return watches_.count(addr) != 0;
}

const std::unordered_map<uint16_t, ExecutionEngine::WatchEntry>& ExecutionEngine::watches() const {
  return watches_;
}

std::optional<std::tuple<uint16_t, uint8_t, uint8_t>> ExecutionEngine::last_watch_trigger() const {
  return last_watch_trigger_;
}

std::optional<std::tuple<uint16_t, uint8_t, uint8_t>> ExecutionEngine::check_watches() {
  for (auto& [addr, entry] : watches_) {
    uint8_t current{0};
    beeb_->get_memory_contents(addr, 1, &current);
    if (current != entry.shadow) {
      const uint8_t old_val = entry.shadow;
      entry.shadow = current;
      if (!entry.trigger_value || current == *entry.trigger_value) {
        return std::make_tuple(addr, old_val, current);
      }
    }
  }
  return std::nullopt;
}

// ─── callback ────────────────────────────────────────────────────────────────

void ExecutionEngine::set_instruction_callback(InstructionCallback cb) {
  callback_ = std::move(cb);
}

// ─── instruction history ─────────────────────────────────────────────────────

std::vector<ExecutionEngine::InsnRecord> ExecutionEngine::instruction_history() const {
  std::vector<InsnRecord> result;
  result.reserve(history_count_);
  // When not full, oldest entry is at index 0.
  // When full, oldest entry is at history_head_ (the slot about to be overwritten).
  const size_t start = (history_count_ < HISTORY_CAPACITY) ? 0 : history_head_;
  for (size_t i = 0; i < history_count_; ++i) {
    result.push_back(history_buf_[(start + i) % HISTORY_CAPACITY]);
  }
  return result;
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
