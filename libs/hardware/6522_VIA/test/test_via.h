#ifndef BEEB_TEST_VIA_H
#define BEEB_TEST_VIA_H

#include <gtest/gtest.h>
#include "6522_via.h"
#include "bus.h"
#include <spdlog/spdlog.h>
#include <memory>

// VIA base address used in all tests
static const uint16_t VIA_BASE = 0xfe40;

// VIA register offsets (from 6522 datasheet)
static const uint8_t VIA_IORB  = 0x00;  // Input/Output Register B
static const uint8_t VIA_IORA  = 0x01;  // Input/Output Register A
static const uint8_t VIA_DDRB  = 0x02;  // Data Direction Register B
static const uint8_t VIA_DDRA  = 0x03;  // Data Direction Register A
static const uint8_t VIA_T1C_L = 0x04;  // T1 Counter Low (write: latch low; read: counter low + clear IRQ)
static const uint8_t VIA_T1C_H = 0x05;  // T1 Counter High (write: load counter + start; read: counter high)
static const uint8_t VIA_T1L_L = 0x06;  // T1 Latch Low (write only)
static const uint8_t VIA_T1L_H = 0x07;  // T1 Latch High (write only, does not reload counter)
static const uint8_t VIA_T2C_L = 0x08;  // T2 Counter Low (write: latch; read: counter low)
static const uint8_t VIA_T2C_H = 0x09;  // T2 Counter High (write: load + start; read: counter high)
static const uint8_t VIA_ACR   = 0x0B;  // Auxiliary Control Register
static const uint8_t VIA_PCR   = 0x0C;  // Peripheral Control Register
static const uint8_t VIA_IFR   = 0x0D;  // Interrupt Flag Register
static const uint8_t VIA_IER   = 0x0E;  // Interrupt Enable Register

// IFR/IER bit masks
static const uint8_t IRQ_CA2 = (1 << 0);
static const uint8_t IRQ_CA1 = (1 << 1);
static const uint8_t IRQ_SR  = (1 << 2);
static const uint8_t IRQ_CB2 = (1 << 3);
static const uint8_t IRQ_CB1 = (1 << 4);
static const uint8_t IRQ_T2  = (1 << 5);
static const uint8_t IRQ_T1  = (1 << 6);
static const uint8_t IRQ_ANY = (1 << 7);  // IFR bit 7: composite IRQ flag

class TestVia : public testing::Test {
 public:
  Via* via_;
  std::shared_ptr<Bus> bus_;

  void SetUp() override;
  void TearDown() override;

  // Write to a VIA register via the bus (one tick: check_timers runs first, then mmio_write)
  void write_reg(uint8_t reg, uint8_t value);

  // Read from a VIA register via the bus (one tick: check_timers runs first, then mmio_read)
  uint8_t read_reg(uint8_t reg);

  // Tick the VIA n times with bus address outside VIA range (timer-only ticks, no MMIO)
  void tick_n(int n);

  // Load T1 counter and start counting (writes T1C_L then T1C_H)
  void load_t1(uint16_t count);

  // Load T2 counter and start counting (writes T2C_L then T2C_H)
  void load_t2(uint16_t count);

  // Enable an IRQ source in IER (IER write with bit 7 set = set mode)
  void enable_irq(uint8_t irq_mask);

  // Clear bits in IFR (write 1 to each bit to clear)
  void clear_ifr(uint8_t mask);
};

#endif  // BEEB_TEST_VIA_H
