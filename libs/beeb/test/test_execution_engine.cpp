#include "test_execution_engine.h"

void TestExecutionEngine::SetUp() {
  engine = std::make_unique<ExecutionEngine>(std::make_unique<Beeb>());
  engine->board().reset();
}

// After reset the CPU is mid-fetch; step_instruction() finishes that fetch and
// runs one instruction, leaving the bus pointing at the NEXT instruction.
TEST_F(TestExecutionEngine, step_advances_to_next_instruction) {
  auto addr_before = engine->board().bus()->get_address();
  engine->step_instruction();
  auto addr_after = engine->board().bus()->get_address();
  EXPECT_NE(addr_before, addr_after);
}

TEST_F(TestExecutionEngine, is_paused_after_step) {
  engine->step_instruction();
  EXPECT_TRUE(engine->is_paused());
}

TEST_F(TestExecutionEngine, callback_fires_once_per_step) {
  int count = 0;
  engine->set_instruction_callback(
      [&](uint16_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint32_t) {
        ++count;
      });
  engine->step_instruction();
  EXPECT_EQ(1, count);
  engine->step_instruction();
  EXPECT_EQ(2, count);
}

TEST_F(TestExecutionEngine, callback_receives_current_pc) {
  uint16_t reported_pc = 0;
  engine->set_instruction_callback(
      [&](uint16_t pc, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint32_t) {
        reported_pc = pc;
      });
  engine->step_instruction();
  // Reported PC must be a plausible ROM address (>= 0x8000).
  EXPECT_GE(reported_pc, 0x8000);
}

TEST_F(TestExecutionEngine, no_callback_when_null) {
  // Should not crash when no callback is installed.
  engine->set_instruction_callback(nullptr);
  EXPECT_NO_THROW(engine->step_instruction());
}

// --- Breakpoint management ---

TEST_F(TestExecutionEngine, add_and_query_breakpoint) {
  engine->add_breakpoint(0x1234);
  EXPECT_TRUE(engine->is_breakpoint(0x1234));
  EXPECT_FALSE(engine->is_breakpoint(0x5678));
}

TEST_F(TestExecutionEngine, remove_breakpoint) {
  engine->add_breakpoint(0xABCD);
  engine->remove_breakpoint(0xABCD);
  EXPECT_FALSE(engine->is_breakpoint(0xABCD));
}

TEST_F(TestExecutionEngine, multiple_breakpoints) {
  engine->add_breakpoint(0x0100);
  engine->add_breakpoint(0x0200);
  engine->add_breakpoint(0x0300);
  EXPECT_TRUE(engine->is_breakpoint(0x0100));
  EXPECT_TRUE(engine->is_breakpoint(0x0200));
  EXPECT_TRUE(engine->is_breakpoint(0x0300));
  EXPECT_FALSE(engine->is_breakpoint(0x0400));
}

// --- run() / breakpoint integration ---

// The breakpoint check in run() fires when, after completing an instruction,
// the bus address (= address of NEXT instruction about to be fetched) matches a
// breakpoint.  To test this deterministically we:
//   1. Step several instructions to learn a future PC sequence.
//   2. Reset the board (deterministic ROM) and set a breakpoint at that address.
//   3. Call run() — it must stop before or at that address.
TEST_F(TestExecutionEngine, run_stops_at_breakpoint) {
  // First pass: learn the address that will appear on the bus after step 4.
  engine->step_instruction(); // step 1
  engine->step_instruction(); // step 2
  engine->step_instruction(); // step 3
  engine->step_instruction(); // step 4
  uint16_t bp_addr = engine->board().bus()->get_address();

  // Second pass: reset board, add breakpoint, run.
  engine->board().reset();
  engine->add_breakpoint(bp_addr);
  engine->resume();
  engine->run();

  EXPECT_TRUE(engine->is_paused());
  EXPECT_EQ(bp_addr, engine->board().bus()->get_address());
}

TEST_F(TestExecutionEngine, run_fires_callback_before_breakpoint) {
  // Discover address after 4 steps (same method as above).
  engine->step_instruction();
  engine->step_instruction();
  engine->step_instruction();
  engine->step_instruction();
  uint16_t bp_addr = engine->board().bus()->get_address();

  engine->board().reset();
  engine->add_breakpoint(bp_addr);

  int count = 0;
  engine->set_instruction_callback(
      [&](uint16_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint32_t) {
        ++count;
      });

  engine->resume();
  engine->run();

  // run() should have called the callback at least 3 times (once per step
  // before hitting the breakpoint on the 4th).
  EXPECT_GE(count, 3);
}

// --- resume / pause state transitions ---

TEST_F(TestExecutionEngine, resume_transitions_to_running) {
  engine->resume();
  EXPECT_FALSE(engine->is_paused());
}

TEST_F(TestExecutionEngine, pause_transitions_to_paused) {
  engine->resume();
  engine->pause();
  EXPECT_TRUE(engine->is_paused());
}
