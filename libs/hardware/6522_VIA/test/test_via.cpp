#include "test_via.h"

#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

// ---------------------------------------------------------------------------
// Fixture setup / teardown
// ---------------------------------------------------------------------------

void TestVia::SetUp() {
  // Pre-register a null-sink logger under the VIA's expected name so the Via
  // constructor doesn't attempt to open a log file (which would fail in a test
  // environment without a logs/ directory).
  const std::string logger_name = "VIA@fe40";
  if (!spdlog::get(logger_name)) {
    auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>(logger_name, null_sink);
    spdlog::register_logger(logger);
  }

  via_ = new Via(VIA_BASE);
  bus_ = std::make_shared<Bus>();
  bus_->set_RW();
  bus_->set_address(0x0000);  // Outside VIA range — timer-only ticks

  // Load T2 to max count so uninitialized timer2_count_ doesn't pollute IFR
  // in tests that don't exercise T2.
  load_t2(0xFFFF);
}

void TestVia::TearDown() {
  delete via_;
  via_ = nullptr;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void TestVia::write_reg(uint8_t reg, uint8_t value) {
  via_->tick_timers();           // advance timers first (matches original tick() order)
  bus_->clr_RW();
  bus_->set_address(VIA_BASE + reg);
  bus_->set_data(value);
  via_->tick(bus_);
  bus_->set_RW();
  bus_->set_address(0x0000);
}

uint8_t TestVia::read_reg(uint8_t reg) {
  via_->tick_timers();           // advance timers first (matches original tick() order)
  bus_->set_RW();
  bus_->set_address(VIA_BASE + reg);
  via_->tick(bus_);
  uint8_t val = bus_->get_data();
  bus_->set_address(0x0000);
  return val;
}

void TestVia::tick_n(int n) {
  for (int i = 0; i < n; ++i)
    via_->tick_timers();
}

void TestVia::load_t1(uint16_t count) {
  write_reg(VIA_T1C_L, count & 0xFF);
  write_reg(VIA_T1C_H, count >> 8);
}

void TestVia::load_t2(uint16_t count) {
  write_reg(VIA_T2C_L, count & 0xFF);
  write_reg(VIA_T2C_H, count >> 8);
}

void TestVia::enable_irq(uint8_t irq_mask) {
  // IER write: bit 7 = 1 → SET mode, bits 0-6 select which sources to enable
  write_reg(VIA_IER, 0x80 | irq_mask);
}

void TestVia::clear_ifr(uint8_t mask) {
  // Write 1 to IFR bits to clear them (bit 7 has no effect)
  write_reg(VIA_IFR, mask & 0x7F);
}

// ===========================================================================
// IER — Interrupt Enable Register
// ===========================================================================

// Reading IER always returns bit 7 set (datasheet §3.5)
TEST_F(TestVia, IER_ReadAlwaysHasBit7Set) {
  EXPECT_EQ(0x80, read_reg(VIA_IER) & 0x80);
}

// Writing IER with bit 7 = 1 sets the indicated bits (SET mode)
TEST_F(TestVia, IER_SetMode_SetsSpecifiedBits) {
  enable_irq(IRQ_T1);  // 0x80 | 0x40 = 0xC0
  uint8_t ier = read_reg(VIA_IER);
  EXPECT_EQ(IRQ_T1, ier & IRQ_T1);
}

// Writing IER with bit 7 = 0 clears the indicated bits (CLEAR mode)
TEST_F(TestVia, IER_ClearMode_ClearsSpecifiedBits) {
  enable_irq(IRQ_T1 | IRQ_T2);           // Set bits 5 and 6
  write_reg(VIA_IER, IRQ_T1);            // bit 7 = 0 → clear bit 6
  uint8_t ier = read_reg(VIA_IER);
  EXPECT_EQ(0, ier & IRQ_T1);            // T1 bit cleared
  EXPECT_EQ(IRQ_T2, ier & IRQ_T2);       // T2 bit unaffected
}

// Enabling multiple sources independently accumulates in IER
TEST_F(TestVia, IER_MultipleEnables_Accumulate) {
  enable_irq(IRQ_T1);
  enable_irq(IRQ_CA1);
  uint8_t ier = read_reg(VIA_IER);
  EXPECT_EQ(IRQ_T1,  ier & IRQ_T1);
  EXPECT_EQ(IRQ_CA1, ier & IRQ_CA1);
}

// ===========================================================================
// IFR — Interrupt Flag Register and has_irq()
// ===========================================================================

// Fresh VIA has no pending interrupts
TEST_F(TestVia, HasIrq_FalseInitially) {
  EXPECT_FALSE(via_->has_irq());
}

// IFR reads as 0x00 when no IRQ is active
TEST_F(TestVia, IFR_ReadsZeroInitially) {
  EXPECT_EQ(0x00, read_reg(VIA_IFR));
}

// T1 fires but IER bit not enabled → has_irq false, IFR bit 6 still set
TEST_F(TestVia, HasIrq_FalseWhenIfrSetButIerDisabled) {
  // IER bit 6 NOT enabled (default is all disabled)
  load_t1(3);
  tick_n(3);  // T1 fires → IFR bit 6 set
  EXPECT_EQ(IRQ_T1, read_reg(VIA_IFR) & IRQ_T1);  // IFR bit is set
  EXPECT_FALSE(via_->has_irq());                   // but no IRQ asserted
}

// IFR bit 7 is the composite IRQ flag: set when any enabled IRQ is active
TEST_F(TestVia, IFR_Bit7SetWhenEnabledIrqActive) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  uint8_t ifr = read_reg(VIA_IFR);
  EXPECT_EQ(IRQ_ANY, ifr & IRQ_ANY);
}

// has_irq() is true when IFR and IER both have a bit in common (bits 0-6)
TEST_F(TestVia, HasIrq_TrueWhenIfrAndIerBothSet) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
}

// Writing 1 to an IFR bit clears it (CPU acknowledges interrupt)
TEST_F(TestVia, IFR_WriteClearsSpecifiedBits) {
  enable_irq(IRQ_T1 | IRQ_T2);
  load_t1(3);
  load_t2(3);
  tick_n(3);
  // Both T1 and T2 fired
  clear_ifr(IRQ_T1);
  uint8_t ifr = read_reg(VIA_IFR);
  EXPECT_EQ(0, ifr & IRQ_T1);         // T1 cleared
  EXPECT_EQ(IRQ_T2, ifr & IRQ_T2);    // T2 still set
}

// has_irq() returns false after all active IRQ sources are cleared
TEST_F(TestVia, HasIrq_FalseAfterClearingAllFlags) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
  clear_ifr(IRQ_T1);
  EXPECT_FALSE(via_->has_irq());
}

// ===========================================================================
// Timer 1 — one-shot mode (default: ACR bit 6 = 0)
// ===========================================================================

// Writing T1C_L alone does not start counting or set IRQ
TEST_F(TestVia, T1_WriteCL_DoesNotStartCounter) {
  write_reg(VIA_T1C_L, 3);
  tick_n(3);
  EXPECT_EQ(0, read_reg(VIA_IFR) & IRQ_T1);
}

// Writing T1C_H clears any existing T1 IRQ flag (datasheet: "clears T1 interrupt")
TEST_F(TestVia, T1_WriteCH_ClearsExistingT1Irq) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());       // T1 fired
  load_t1(100);                       // Reload clears flag
  EXPECT_FALSE(via_->has_irq());
}

// T1 fires after exactly `count` timer ticks (each tick is one Via::tick call).
// NB: read_reg() itself consumes a tick (it calls via_->tick), so has_irq() must
//     be used for the "not yet fired" check to avoid an unintended extra tick.
TEST_F(TestVia, T1_FiresAfterExactCountTicks) {
  enable_irq(IRQ_T1);
  load_t1(5);
  tick_n(4);                // Four ticks: counter 5→4→3→2→1
  EXPECT_FALSE(via_->has_irq());  // Not yet fired (non-ticking check)
  tick_n(1);                // Fifth tick: counter 1→0 → fires
  EXPECT_TRUE(via_->has_irq());
  EXPECT_EQ(IRQ_T1, read_reg(VIA_IFR) & IRQ_T1);
}

// T1 sets IFR bit 6 on underflow
TEST_F(TestVia, T1_SetsIfrBit6OnUnderflow) {
  load_t1(3);
  tick_n(3);
  EXPECT_EQ(IRQ_T1, read_reg(VIA_IFR) & IRQ_T1);
}

// has_irq() is true after T1 fires and IER bit 6 is enabled
TEST_F(TestVia, T1_HasIrqTrue_WhenIerEnabled) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
}

// Reading T1C_L clears the T1 IRQ flag (datasheet: read T1C_L clears interrupt)
TEST_F(TestVia, T1_ReadT1CL_ClearsT1IrqFlag) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
  read_reg(VIA_T1C_L);               // Reading T1C_L should clear T1 flag
  EXPECT_FALSE(via_->has_irq());
  EXPECT_EQ(0, read_reg(VIA_IFR) & IRQ_T1);
}

// One-shot mode: T1 does not reload after firing (counter stops at 0)
TEST_F(TestVia, T1_OneShotDoesNotReloadAfterFire) {
  enable_irq(IRQ_T1);
  load_t1(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
  clear_ifr(IRQ_T1);
  EXPECT_FALSE(via_->has_irq());
  tick_n(20);                         // Many more ticks — should not refire
  EXPECT_FALSE(via_->has_irq());
}

// ===========================================================================
// Timer 1 — continuous mode (ACR bit 6 = 1)
// ===========================================================================

// In continuous mode T1 reloads from latch and fires again
TEST_F(TestVia, T1_ContinuousMode_ReloadsAndRefires) {
  write_reg(VIA_ACR, 0x40);          // Continuous mode (bit 6)
  enable_irq(IRQ_T1);
  load_t1(3);

  // First firing
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
  clear_ifr(IRQ_T1);
  EXPECT_FALSE(via_->has_irq());

  // Second firing (counter reloaded from latch = 3)
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
}

// IRQ fires multiple times in continuous mode
TEST_F(TestVia, T1_ContinuousMode_IrqFiresRepeatedly) {
  write_reg(VIA_ACR, 0x40);
  enable_irq(IRQ_T1);
  load_t1(4);

  int fire_count = 0;
  for (int i = 0; i < 20; ++i) {
    tick_n(1);
    if (via_->has_irq()) {
      ++fire_count;
      clear_ifr(IRQ_T1);
    }
  }
  EXPECT_GE(fire_count, 2);           // Should fire at least twice in 20 ticks with period=4
}

// T1L_H write updates latch only, does not restart the counter
TEST_F(TestVia, T1_WriteLatcHH_UpdatesLatch_NotCounter) {
  load_t1(10);
  tick_n(5);                          // Counter is at ~5
  write_reg(VIA_T1L_H, 0x01);        // Update latch high byte only
  tick_n(5);                          // Counter continues from ~5, fires at 10
  EXPECT_EQ(IRQ_T1, read_reg(VIA_IFR) & IRQ_T1);
}

// ===========================================================================
// Timer 2 — one-shot mode (default: ACR bit 5 = 0)
// ===========================================================================

// T2 fires after its count expires
TEST_F(TestVia, T2_FiresAfterCountdown) {
  load_t2(5);
  tick_n(5);
  EXPECT_EQ(IRQ_T2, read_reg(VIA_IFR) & IRQ_T2);
}

// T2 sets IFR bit 5 on underflow
TEST_F(TestVia, T2_SetsIfrBit5OnUnderflow) {
  enable_irq(IRQ_T2);
  load_t2(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
}

// Writing T2C_H clears the T2 IRQ flag
TEST_F(TestVia, T2_WriteCH_ClearsT2IrqFlag) {
  enable_irq(IRQ_T2);
  load_t2(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
  load_t2(100);                       // Reload clears flag
  EXPECT_FALSE(via_->has_irq());
}

// One-shot: T2 does not refire after initial timeout
TEST_F(TestVia, T2_OneShotDoesNotRefire) {
  enable_irq(IRQ_T2);
  load_t2(3);
  tick_n(3);
  EXPECT_TRUE(via_->has_irq());
  clear_ifr(IRQ_T2);
  tick_n(20);                         // Well within the 65535-tick wrap
  EXPECT_FALSE(via_->has_irq());
}

// ===========================================================================
// CA1 — edge detection and latching
// ===========================================================================

// CA1 positive edge triggers IRQ_CA1 when PCR bit 0 = 1
// (Positive edge mode is the one that currently works in the implementation)
TEST_F(TestVia, CA1_PositiveEdge_TriggersIrq) {
  write_reg(VIA_PCR, 0x01);          // CA1 positive active edge
  // ca1_ starts at 0; set_ca1(1) = positive edge
  via_->set_ca1(1);
  EXPECT_EQ(IRQ_CA1, read_reg(VIA_IFR) & IRQ_CA1);
}

// CA1 positive edge does not trigger IRQ on negative edge when in positive-edge mode
TEST_F(TestVia, CA1_PositiveEdgeMode_NegativeEdge_NoIrq) {
  write_reg(VIA_PCR, 0x01);
  via_->set_ca1(1);                  // Positive edge → IRQ
  clear_ifr(IRQ_CA1);
  via_->set_ca1(0);                  // Negative edge → should NOT trigger
  EXPECT_EQ(0, read_reg(VIA_IFR) & IRQ_CA1);
}

// CA1 negative edge triggers IRQ_CA1 when PCR bit 0 = 0 (default)
// NOTE: The current implementation has a bug in set_ca1() — it compares
// state against prev_ca1_ instead of ca1_, which means the 1→0 transition
// is incorrectly skipped. This test documents the CORRECT behaviour and is
// expected to FAIL until the bug is fixed.
TEST_F(TestVia, CA1_NegativeEdge_TriggersIrq) {
  // PCR bit 0 = 0 → negative active edge (default)
  via_->set_ca1(1);                  // Go high (no IRQ in negative-edge mode)
  EXPECT_EQ(0, read_reg(VIA_IFR) & IRQ_CA1);
  via_->set_ca1(0);                  // Negative edge → SHOULD trigger IRQ_CA1
  EXPECT_EQ(IRQ_CA1, read_reg(VIA_IFR) & IRQ_CA1);
}

// Reading Port A clears IRQ_CA1 (datasheet: reading IORA acknowledges CA1)
TEST_F(TestVia, CA1_ReadPortA_ClearsIrqCA1) {
  write_reg(VIA_PCR, 0x01);
  enable_irq(IRQ_CA1);
  via_->set_ca1(1);
  EXPECT_TRUE(via_->has_irq());
  read_reg(VIA_IORA);                // Reading Port A clears CA1 flag
  EXPECT_EQ(0, read_reg(VIA_IFR) & IRQ_CA1);
  EXPECT_FALSE(via_->has_irq());
}

// CA1 active edge latches Port A when ACR bit 0 = 1
TEST_F(TestVia, CA1_Latch_CapturesPortAOnActiveEdge) {
  write_reg(VIA_PCR, 0x01);         // Positive active edge
  write_reg(VIA_ACR, 0x01);         // PA latch enable

  // Provide known data on Port A via a provider
  auto provider = std::make_shared<data_provider_8_bit>(0x5A);
  provider->provide_data(0x5A);
  via_->provide_port_a(provider);

  via_->set_ca1(1);                  // Active edge — latches PA

  // Read IORA — with latch enabled, returns the latched value
  uint8_t pa = read_reg(VIA_IORA);
  EXPECT_EQ(0x5A, pa);
}

// ===========================================================================
// CB1 — edge detection
// ===========================================================================

// CB1 negative edge triggers IRQ_CB1 when PCR bit 4 = 0 (default)
// NOTE: The current implementation has two bugs affecting CB1:
// (1) Same prev_cb1_ comparison bug as CA1
// (2) The active-edge check uses PCR_CB1_IRQ_CTL (0x10) directly compared
//     against cb1_ (0 or 1) — so the positive-edge mode check is always false.
// This test documents CORRECT behaviour and is expected to FAIL until fixed.
TEST_F(TestVia, CB1_NegativeEdge_TriggersIrq) {
  // PCR bit 4 = 0 → negative active edge (default)
  via_->set_cb1(1);                  // Go high
  EXPECT_EQ(0, read_reg(VIA_IFR) & IRQ_CB1);
  via_->set_cb1(0);                  // Negative edge → SHOULD trigger IRQ_CB1
  EXPECT_EQ(IRQ_CB1, read_reg(VIA_IFR) & IRQ_CB1);
}

// CB1 positive edge triggers IRQ_CB1 when PCR bit 4 = 1
// NOTE: Also expected to FAIL due to the PCR_CB1_IRQ_CTL bitmask bug.
TEST_F(TestVia, CB1_PositiveEdge_TriggersIrq) {
  write_reg(VIA_PCR, 0x10);         // CB1 positive active edge
  via_->set_cb1(1);                  // Positive edge → SHOULD trigger IRQ_CB1
  EXPECT_EQ(IRQ_CB1, read_reg(VIA_IFR) & IRQ_CB1);
}

// ===========================================================================
// Port A — data direction and I/O
// ===========================================================================

// All-input port reads from external provider
TEST_F(TestVia, PortA_AllInputs_ReadFromProvider) {
  write_reg(VIA_DDRA, 0x00);        // All inputs
  auto provider = std::make_shared<data_provider_8_bit>();
  provider->provide_data(0x5A);
  via_->provide_port_a(provider);
  uint8_t pa = read_reg(VIA_IORA);
  EXPECT_EQ(0x5A, pa);
}

// Mixed direction: output bits reflect ORA, input bits reflect provider
TEST_F(TestVia, PortA_MixedDirection_OutputFromORA_InputFromProvider) {
  write_reg(VIA_DDRA, 0xF0);        // High nibble output, low nibble input
  write_reg(VIA_IORA, 0xA0);        // Set ORA high nibble = 0xA
  auto provider = std::make_shared<data_provider_8_bit>();
  provider->provide_data(0x05);      // Provider drives low nibble = 0x5
  via_->provide_port_a(provider);
  uint8_t pa = read_reg(VIA_IORA);
  EXPECT_EQ(0xA5, pa);
}

// All-output port: reading back should return ORA
// NOTE: The current implementation uses (ora_ | ~ddra_) & ira_ which returns 0
// when ira_ = 0 (no external loopback provider). This test documents CORRECT
// behaviour per the 6522 datasheet and is expected to FAIL until fixed.
TEST_F(TestVia, PortA_AllOutputs_ReadBackORA) {
  write_reg(VIA_DDRA, 0xFF);        // All outputs
  write_reg(VIA_IORA, 0xAB);        // Set ORA
  uint8_t pa = read_reg(VIA_IORA);
  EXPECT_EQ(0xAB, pa);
}

// ===========================================================================
// Port B — data direction and I/O
// ===========================================================================

// All-output port B: reading should return ORB
// NOTE: Same read_port_b issue as Port A — expected to FAIL until fixed.
TEST_F(TestVia, PortB_AllOutputs_ReadBackORB) {
  write_reg(VIA_DDRB, 0xFF);
  write_reg(VIA_IORB, 0xCD);
  uint8_t pb = read_reg(VIA_IORB);
  EXPECT_EQ(0xCD, pb);
}
