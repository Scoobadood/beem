#pragma once

#include "beeb.h"
#include <atomic>
#include <functional>
#include <memory>
#include <set>

class ExecutionEngine {
 public:
  explicit ExecutionEngine(std::unique_ptr<Beeb> beeb);

  // Execute exactly one instruction (SYNC-boundary to SYNC-boundary).
  // Fires the callback if one is installed.  Always leaves state == PAUSED.
  void step_instruction();

  // Run until pause() is called from another thread, a breakpoint is hit,
  // or step_out reaches its target.  Returns when paused.
  void run();

  // Read the return address from the stack and run until execution returns
  // to that address.  No-op unless currently PAUSED.
  void step_out();

  // Thread-safe: transitions RUNNING or STEPPING_OUT → PAUSED.
  void pause();

  // Transition PAUSED → RUNNING.  Must be called before run().
  void resume();

  // Breakpoint set —————————————————————————————————————————————
  void add_breakpoint(uint16_t addr);
  void remove_breakpoint(uint16_t addr);
  bool is_breakpoint(uint16_t addr) const;
  const std::set<uint16_t>& breakpoints() const;

  // Observation callback ————————————————————————————————————————
  // Fires after each completed instruction with: pc, A, X, Y, flags, SP,
  // and the 4 bytes of memory starting at pc (packed little-endian).
  // Pass nullptr to disable.
  using InstructionCallback = std::function<
      void(uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
           uint8_t flags, uint8_t sp, uint32_t mem4)>;
  void set_instruction_callback(InstructionCallback cb);

  // Board access ————————————————————————————————————————————————
  Beeb& board();
  const Beeb& board() const;
  bool is_paused() const;

 private:
  // Execute one instruction: drain current opcode fetch then run to next SYNC.
  void tick_one_instruction();

  // Fire the callback with current CPU state (no-op if no callback installed).
  void fire_callback();

  static constexpr int32_t PAUSED       = 0;
  static constexpr int32_t STEPPING_OUT = 1;
  static constexpr int32_t RUNNING      = 2;

  std::unique_ptr<Beeb>    beeb_;
  std::atomic<int32_t>     state_{PAUSED};
  std::set<uint16_t>       breakpoints_;
  uint16_t                 step_out_target_{0};
  InstructionCallback      callback_;
};
