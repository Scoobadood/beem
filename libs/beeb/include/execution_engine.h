#pragma once

#include "beeb.h"
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

class ExecutionEngine {
 public:
  explicit ExecutionEngine(std::unique_ptr<Beeb> beeb);

  // ── Instruction history ───────────────────────────────────────────────────
  struct InsnRecord {
    uint16_t pc{0};
    uint8_t  a{0}, x{0}, y{0}, sp{0}, flags{0};
    uint32_t mem4{0};  // 4 bytes at PC (little-endian) for disassembly
  };
  static constexpr size_t HISTORY_CAPACITY = 100;

  // Returns up to HISTORY_CAPACITY records in execution order (oldest first).
  std::vector<InsnRecord> instruction_history() const;

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

  // Watch breakpoints ———————————————————————————————————————————
  // Stops execution when the watched address changes value.
  // If trigger_value is set, only stops when the new value matches it.
  struct WatchEntry {
    uint8_t shadow{0};
    std::optional<uint8_t> trigger_value;
  };

  void add_watch(uint16_t addr, std::optional<uint8_t> trigger_value = std::nullopt);
  void remove_watch(uint16_t addr);
  bool is_watch(uint16_t addr) const;
  const std::unordered_map<uint16_t, WatchEntry>& watches() const;

  // If the most recent run() paused due to a watch, returns {addr, old, new}.
  // Cleared by the next call to run() or resume().
  std::optional<std::tuple<uint16_t, uint8_t, uint8_t>> last_watch_trigger() const;

  // Observation callback ————————————————————————————————————————
  // Fires after each completed instruction with: pc, A, X, Y, flags, SP,
  // and the 4 bytes of memory starting at pc (packed little-endian).
  // Pass nullptr to disable.
  using InstructionCallback = std::function<
      void(uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
           uint8_t flags, uint8_t sp, uint32_t mem4)>;
  void set_instruction_callback(InstructionCallback cb);

  // Logpoints ————————————————————————————————————
  // When PC hits a logpoint, fires callback with current CPU state plus the
  // optional memory address and its current value.  Execution is NOT paused.
  using LogpointCallback = std::function<
      void(uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
           uint8_t flags, uint8_t sp, uint32_t mem4,
           std::optional<uint16_t> mem_addr, uint8_t mem_val)>;
  void set_logpoint_callback(LogpointCallback cb);

  void add_logpoint(uint16_t pc_addr, std::optional<uint16_t> mem_addr = std::nullopt);
  void remove_logpoint(uint16_t pc_addr);
  bool is_logpoint(uint16_t pc_addr) const;

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

  // Checks each watched address against its shadow.  Returns {addr, old, new}
  // for the first address whose value has changed, and updates the shadow.
  std::optional<std::tuple<uint16_t, uint8_t, uint8_t>> check_watches();

  std::unique_ptr<Beeb>    beeb_;
  std::atomic<int32_t>     state_{PAUSED};
  std::set<uint16_t>       breakpoints_;
  std::unordered_map<uint16_t, WatchEntry> watches_;
  std::optional<std::tuple<uint16_t, uint8_t, uint8_t>> last_watch_trigger_;
  uint16_t                 step_out_target_{0};
  InstructionCallback      callback_;
  LogpointCallback         logpoint_callback_;
  std::unordered_map<uint16_t, std::optional<uint16_t>> logpoints_;

  std::array<InsnRecord, HISTORY_CAPACITY> history_buf_{};
  size_t history_head_{0};
  size_t history_count_{0};
};
