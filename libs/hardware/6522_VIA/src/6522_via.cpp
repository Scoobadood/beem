#include "6522_via.h"
#include "bus.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/spdlog.h>

const uint8_t IORB = 0x00;
const uint8_t IORA = 0x01;
const uint8_t DDRB = 0x02;
const uint8_t DDRA = 0x03;
const uint8_t T1C_L = 0x04;
const uint8_t T1C_H = 0x05;
const uint8_t T1L_L = 0x06;
const uint8_t T1L_H = 0x07;
const uint8_t T2C_L = 0x08;
const uint8_t T2C_H = 0x09;
const uint8_t SR = 0x0A;
const uint8_t ACR = 0x0B;
const uint8_t PCR = 0x0C;
const uint8_t IFR = 0x0D;
const uint8_t IER = 0x0E;
const uint8_t IORA_NOH = 0x0F;

/* IRQ */
const uint8_t IRQ_CA2 = (0x01 << 0);
const uint8_t IRQ_CA1 = (0x01 << 1);
const uint8_t IRQ_SR = (0x01 << 2);
const uint8_t IRQ_CB2 = (0x01 << 3);
const uint8_t IRQ_CB1 = (0x01 << 4);
const uint8_t IRQ_T2 = (0x01 << 5);
const uint8_t IRQ_T1 = (0x01 << 6);
const uint8_t IRQ_IRQ = (0x01 << 7);

/* PCR */
const uint8_t PCR_CA1_IRQ_CTL = 0x01;
const uint8_t PCR_CB1_IRQ_CTL = 0x10;
#define PCR_CA2_CLR_RW_A(pcr) (((pcr & 0x0e) != 0x02) && ((pcr & 0x0e) != 0x06))

/* ACR */
const uint8_t ACR_PA_LATCH = (0x01 << 0);
const uint8_t ACR_PB_LATCH = (0x01 << 1);
#define PA_LATCHED(c) (TST_FLG(c, ACR_PA_LATCH))
#define PB_LATCHED(c) (TST_FLG(c, ACR_PB_LATCH))

#define ACR_T1_PB7(c) (TST_FLG(c, 0x80))
#define ACR_T1_CTL(c) (TST_FLG(c, 0x40))
#define ACR_T2_CTL(c) (TST_FLG(c, 0x20))

#define TST_FLG(data, flag) ((data & flag) == flag)
#define TST_T1(data) TST_FLG(data, IRQ_T1)
#define TST_T2(data) TST_FLG(data, IRQ_T2)
#define TST_CA1(data) TST_FLG(data, IRQ_CA1)
#define TST_CA2(data) TST_FLG(data, IRQ_CA2)
#define TST_CB1(data) TST_FLG(data, IRQ_CB1)
#define TST_CB2(data)  TST_FLG(data, IRQ_CB2)
#define TST_SR(data)  TST_FLG(data, IRQ_SR)

Via::Via(uint16_t base_address) //
    : base_address_{base_address} //
    , ddra_{0} //
    , ira_{0} //
    , ora_{0} //
    , ca1_{0} //
    , ca2_{0} //
    , pa_latch_{0} //
    , prev_ca1_{0} //
    , ddrb_{0} //
    , irb_{0} //
    , orb_{0} //
    , cb1_{0} //
    , cb2_{0} //
    , pb_latch_{0} //
    , prev_cb1_{0} //
    , ier_{0} //
    , ifr_{0} //
    , acr_{0} //
    , pcr_{0} //
    , ca1_pos_active_edge_{false} //
    , cb1_pos_active_edge_{false} //
    , ca2_ctl_{0} //
    , cb2_ctl_{0} //
    , timer1_count_{0} //
    , timer1_latch_{0} //
    , pb7_{0} //
{
  via_name_ = fmt::format("VIA@{:04x}", base_address);
  if (!spdlog::get(via_name_)) {
    try {
      auto logger = spdlog::basic_logger_mt(via_name_, "logs/" + via_name_ + ".txt", true);
      logger->flush_on(spdlog::level::err);
    }
    catch (const spdlog::spdlog_ex &ex) {
      spdlog::error("Log init failed: {}", ex.what());
    }
  }
  logger_ = spdlog::get(via_name_);
}

void Via::check_mmio(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < base_address_) {
    return;
  }

  addr -= base_address_;
  if (addr > 0x0F) {
    return;
  }

  if (bus->tst_RW()) {
    mmio_read(bus, addr);
  } else {
    mmio_write(bus, addr);
  }
}

void Via::mmio_read(const std::shared_ptr<Bus> &bus, uint8_t reg) {
  uint8_t data = bus->get_data();
  switch (reg) {
    case IORB:
      if (PB_LATCHED(acr_)) {
        data = pb_latch_;
      } else {
        data = read_port_b();
      }
      logger_->info("Read ({:02x}) from {} IRB",
                                   data,
                                   PB_LATCHED(acr_) ? "(latched)" : "");
      break;

    case IORA:
    case IORA_NOH:
      if (PA_LATCHED(acr_)) {
        data = pa_latch_;
      } else {
        data = read_port_a();
      }
      logger_->info("Read ({:02x}) from {} IRA{}",
                                   data,
                                   PA_LATCHED(acr_) ? "(latched)" : "",
                                   (reg == IORA ? "" : "_NOH"));
      clear_irq(IRQ_CA1);
      break;

    case DDRB:
      data = ddrb_;
      logger_->info("Read ({:02x}) from DDRB", data);
      break;

    case DDRA:
      data = ddra_;
      logger_->info("Read ({:02x}) from DDRA", data);
      break;

    case T1C_L:
      logger_->info("Read T1C_L");
      clear_irq(IRQ_T1);
      break;
    case T1C_H:
      logger_->info("Read T1C_H");
      break;
    case T1L_L:
      logger_->info("Read T1L_L");
      break;
    case T1L_H:
      logger_->info("Read T1L_H");
      break;
    case T2C_L:
      logger_->info("Read T2C_L");
      break;
    case T2C_H:
      logger_->info("Read T2C_H");
      break;
    case SR:
      logger_->info("Read SR");
      break;

    case ACR:
      data = acr_;
      break;

    case PCR:
      data = pcr_;
      break;

    case IFR:
      if (ifr_ & 0x7f) data = ifr_ | 0x80;
      else data = 0;
      break;

    case IER:
      /**
       * Reading:
       * Bits 0-6 are read as expected.
       * Bit 7 is always set when read.
       */
      data = (ier_ | 0x80);
      break;

    default:
      spdlog::critical("Read Unknown register ({:02x})", reg);
      break;
  }
  bus->set_data(data);
}

/**
 * Read the pins of Port A by polling providers.
 * @return
 */
uint8_t Via::read_port_a() {
  auto data_fetched = 0;
  uint8_t out;
  for (auto &provider : port_a_providers_) {
    if (provider->has_data()) {
      out = (~ddra_ & provider->data()) | (ora_ & ddra_);
      ++data_fetched;
    }
  }
  if (data_fetched == 0) {
    out = (ora_ & ddra_) | (ira_ & ~ddra_);
  }
  if (data_fetched > 1) {
    spdlog::error("Via6522: Multiple data providers read from PortA");
  }

  return out;
}

void Via::write_port_a(uint8_t data) {
  ora_ = data;
  if (ddra_) {
    uint8_t pa = (ora_ & ddra_) | ~ddra_;

    // TODO: Handle pulsed output from timer

    /* Pulse output */
    if (ca2_ctl_ == 0x05) {
      // TODO: Pulsed output, pull CA2 low for a cycle
    } else if (ca2_ctl_ == 0x04) {
      // TODO: Handshake. Pull CA2 low until data taken (CA1)
    }

    /* If CA2 is not "Independent" the write clears CA2 IRQ */
    if (PCR_CA2_CLR_RW_A(pcr_)) {
      clear_irq(IRQ_CA2);
    }

    notify_subscribers(port_a_subscribers_, pa, ddra_);
  }
}

/**
 * Read the pins of Port B by polling providers.
 * @return
 */
uint8_t Via::read_port_b() {
  auto data_fetched = 0;
  uint8_t out;
  for (auto &provider : port_b_providers_) {
    if (provider->has_data()) {
      out = (~ddrb_ & provider->data()) | (orb_ & ddrb_);
      ++data_fetched;
    }
  }
  if (data_fetched == 0) {
    out = (orb_ & ddrb_) | (irb_ & ~ddrb_);
  }
  if (data_fetched > 1) {
    spdlog::error("{}: Multiple data providers read from PortB", via_name_);
  }
  // TODO: If T2 is in pulse counting mode, decrement it eah time PB6 is pulsed low then high
  return out;
}

void Via::write_port_b(uint8_t data) {
  orb_ = data;
  if (ddrb_) {
    uint8_t pb = (orb_ & ddrb_) | ~ddrb_;

    if (ACR_T1_PB7(acr_)) {
      pb = (pb & 0x7f) | (pb7_ << 7);
    }

    /* Pulse output */
    if (cb2_ctl_ == 0x05) {
      // TODO: Pulsed output, pull CB2 low for a cycle
    } else if (cb2_ctl_ == 0x04) {
      // TODO: Handshake. Pull CB2 low until data taken (CB1)
    }

    notify_subscribers(port_b_subscribers_, pb, ddrb_);
  }
}

void Via::write_irq_enable(uint8_t data) {
  logger_->info("Writing ({:02x}) to IER", data);
  auto old_ier = ier_;
  if (data & 0x80) {
    ier_ |= data;
  } else {
    ier_ &= ~(data | 0x80);
  }

  // FIXME: Only report CA2 for debug purposes
  logger_->info("write to IER. {}",
                               ((ier_ ^ old_ier) & IRQ_CA2) ? (TST_CA2(ier_) ? "CA2 enabled" : "CA2 disabled")
                                                            : "CA2 unchanged"
  );
}

void Via::write_acr(uint8_t data) {
  logger_->info("Writing ({:02x}) to ACR", data);

  acr_ = data;

  logger_->info("  PA_L {}", PA_LATCHED(acr_) ? "enabled" : "disabled");
  logger_->info("  PB_L {}", PB_LATCHED(acr_) ? "enabled" : "disabled");
  logger_->info("  T1 {}", ACR_T1_CTL(acr_) ? "Continuous" : "One shot");
  logger_->info("  T2 {}", ACR_T2_CTL(acr_) ? "Count down" : "One shot");
  logger_->info("  PB7 {}", ACR_T1_PB7(acr_) ? "Enabled" : "Disabled");
  switch ((acr_ >> 2) & 0x7) {
    case 0:
      logger_->info("  SR Disabled");
      break;
    case 1:
      logger_->info("  SR Shift in T2");
      break;
    case 2:
      logger_->info("  SR Shift in 1MHz");
      break;
    case 3:
      logger_->info("  SR Shift in Ext Clk");
      break;
    case 4:
      logger_->info("  SR Shift out Free running T2");
      break;
    case 5:
      logger_->info("  SR Shift out T2");
      break;
    case 6:
      logger_->info("  SR Shift out 1MHz");
      break;
    case 7:
      logger_->info("  SR Shift out Ext Clk");
      break;
  }
}

void Via::write_pcr(uint8_t data) {
  logger_->info("Writing ({:02x}) to PCR", data);
  ca1_pos_active_edge_ = TST_FLG(data, PCR_CA1_IRQ_CTL);
  cb1_pos_active_edge_ = TST_FLG(data, PCR_CB1_IRQ_CTL);
  ca2_ctl_ = (data >> 1) & 0x07;
  cb2_ctl_ = (data >> 5) & 0x07;
  pcr_ = data;

  if (ca2_ctl_ & 0x06) {
    ca2_ = ca2_ctl_ & 0x01;
  }
  if (cb2_ctl_ & 0x06) {
    cb2_ = cb2_ctl_ & 0x01;
  }

  logger_->info("  CA1:{} active edge", cb1_pos_active_edge_ ? "positive" : "negative");
  logger_->info("  CB1:{} active edge", cb1_pos_active_edge_ ? "positive" : "negative");
  logger_->info("  CA2_CTL1:{}", ca2_ctl_);
  logger_->info("  CB2_CTL1:{}", cb2_ctl_);
}

void Via::mmio_write(const std::shared_ptr<Bus> &bus, uint8_t reg) {
  auto data = bus->get_data();
  switch (reg) {
    case IORB:
      write_port_b(data);
      break;

    case IORA:
    case IORA_NOH:
      write_port_a(data);
      clear_irq(IRQ_CA1);
      break;

    case DDRB:
      ddrb_ = data;
      break;

    case DDRA:
      ddra_ = data;
      break;

    case T1C_L:
      timer1_latch_ = (timer1_latch_ & 0xff00) | data;
      break;

    case T1C_H:
      timer1_latch_ = (timer1_latch_ & 0xff) | (data << 8);
      timer1_count_ = timer1_latch_;
      if (ACR_T1_PB7(acr_)) pb7_ = 0;
      clear_irq(IRQ_T1);
      break;

    case T1L_L:
      timer1_latch_ = (timer1_latch_ & 0xff00) | data;
      break;

    case T1L_H:
      timer1_latch_ = (timer1_latch_ & 0xff) | (data << 8);
      break;

    case T2C_L:
      timer2_latch_ = data;
      break;

    case T2C_H:
      timer2_count_ = (data << 8) | timer2_latch_;
      clear_irq(IRQ_T2);
      break;

    case SR:
      logger_->info("Wrote ({:02x}) to SR", data);
      break;

    case ACR:
      write_acr(data);
      break;

    case PCR:
      write_pcr(data);
      break;

      /*
       * In the R6522, all the interrupt flags are contained in one register, i.e., the IFR.
       *
       * Interrupt flags are set in the IFR by conditions detected within the R6522 or on inputs to the R6522.
       * These flags normally remain set until the interrupt has been serviced.
       *
       * In addition, bit 7 of this register wil be read as a logic 1 when an interrupt exists
       * within the chip.
       *
       * The Interrupt Flag Register (IRF) may be read directly by the processor.
       * In addition, individual flag bits may be cleared by writing a "1" into the appropriate bit of the IFR.
       */
    case IFR: {
      uint8_t mask = 0x01;
      for (auto bit = 0; bit < 7; ++bit, mask <<= 1) {
        if (data & mask)
          clear_irq(mask);
      }
    }
      break;

    case IER:
      write_irq_enable(data);
      break;

    default:
      spdlog::error("{} Wrote ({:02x}) to unknown register ({:02x})", via_name_, data, reg);
      break;
  }
}

/**
 * See if CA1 is pulled high or low and latching is enabled.
 * If so, latch the data and raise an IRQ
 * https://lateblt.tripod.com/bit67.txt
 */
void Via::set_ca1(uint8_t state) {
  state &= 0x01;
  if (state == ca1_) return;

  prev_ca1_ = ca1_;
  ca1_ = state;

  logger_->debug("set_ca1({:02x}) value changed", state);

  // If PCR_CA1_IRQ_CTL is set then +ve active edge else -ve active edge
  // This tests whether CA1 is triggered
  if (ca1_ == (pcr_ & PCR_CA1_IRQ_CTL)) {

    // If Aux Ctl Reg has PA_LATCH enabled (1)
    // We latch the port A value
    if (acr_ & ACR_PA_LATCH)
      pa_latch_ = read_port_a();


    // CA1 active edge triggered so raise IRQ
    logger_->debug(" raising IRQ CA1");
    raise_irq(IRQ_CA1);
  }
}

/**
 * See if CB1 is pulled high or low and latching is enabled.
 * If so, latch the data and raise an IRQ
 * https://lateblt.tripod.com/bit67.txt
 */
void Via::set_cb1(uint8_t state) {
  state &= 0x01;
  if (state == cb1_) return;
  prev_cb1_ = cb1_;
  cb1_ = state;

  if (cb1_ == ((pcr_ >> 4) & 0x01)) {
    // CB1 went active. Generate IRQ and latch data if enabled
    logger_->info("  CB1 went active");
    if (acr_ & ACR_PB_LATCH)
      pb_latch_ = read_port_b();

    raise_irq(IRQ_CB1);
  }
}

/* Private convenience method to centralise raising IRQs in IFR
 * from 6522 activities.
 * */
void Via::raise_irq(uint8_t irq) {
  // Ignore if it's already raised
  if (TST_FLG(ifr_, irq)) return;

  logger_->debug("raise_irq({:08b}) raised", irq);
  ifr_ |= (irq | IRQ_IRQ);
}

bool Via::has_irq() const {
  bool has = ((ifr_ & ier_ & 0x7f) != 0);
  logger_->debug("has_irq() (== {})", has);
  logger_->debug("          ifr : {:08b} {}", ifr_, (ifr_ != 0) ? "Interrupts present" : "");
  logger_->debug("          ier : {:08b} {}", ier_, (ier_ != 0) ? "Interrupts enabled" : "");
  logger_->debug("          trg : {:08b}", ier_ & ifr_);
  return has;
}

void Via::clear_irq(uint8_t irq) {
  if (!TST_FLG(ifr_, irq)) return;
  logger_->debug("clear_irq({:08b}) cleared", irq);
  ifr_ &= ~irq;
  if (ifr_ == IRQ_IRQ) ifr_ = 0;
}

/**
 * If T1 or T2 are running, update them and handle any appropriate
 * IRQs, relatching etc.
 */
void Via::check_timers() {
  if (timer1_count_ != 0) {
    --timer1_count_;
    if (timer1_count_ == 0) {
      /* If continuous interrupts */
      if (ACR_T1_CTL(acr_)) {
        if (ACR_T1_PB7(acr_)) {
          pb7_ = 1 - pb7_;
        }
        timer1_count_ = timer1_latch_;
      } else {
        /* One shot */
        if (ACR_T1_PB7(acr_)) pb7_ = 1;
      }
      raise_irq(IRQ_T1);
    }
  }

  // T2
  if (!ACR_T2_CTL(acr_)) {
    --timer2_count_;
    if (timer2_count_ == 0) {
      if (!TST_T2(ifr_)) {
        raise_irq(IRQ_T2);
      }
    }
  }
}

void Via::check_ca2() {
  for (const auto &provider : ca2_providers_) {
    if (provider->has_data()) {
      auto data = provider->data();
      if (data == 0x00) {
        raise_irq(IRQ_CA2);
      }
      return;
    }
  }
}

void Via::check_ca1() {
  for (const auto &provider : ca1_providers_) {
    if (provider->has_data()) {
      auto data = provider->data();
      logger_->debug("CA1 data arrived : 0x{:02x}", data);
      set_ca1(data);
    }
  }
}

void Via::tick(const std::shared_ptr<Bus> &bus) {
  check_timers();

  check_ca1();
  check_ca2();

  check_mmio(bus);
}

void Via::provide_port_a(const data_provider_8_bit_ptr &provider) {
  port_a_providers_.emplace(provider);
}

void Via::provide_port_b(const data_provider_8_bit_ptr &provider) {
  port_b_providers_.emplace(provider);
}

void Via::provide_ca2(const data_provider_8_bit_ptr &provider) {
  ca2_providers_.emplace(provider);
}

void Via::provide_ca1(const data_provider_8_bit_ptr &provider) {
  ca1_providers_.emplace(provider);
}

void Via::subscribe_port_b(const data_subscriber_8_bit_ptr &subscriber) {
  port_b_subscribers_.emplace(subscriber);
}

void Via::subscribe_port_a(const data_subscriber_8_bit_ptr &subscriber) {
  port_a_subscribers_.emplace(subscriber);
}

void Via::unsubscribe_port_b(const data_subscriber_8_bit_ptr &subscriber) {
  port_b_subscribers_.erase(subscriber);
}

void Via::unsubscribe_port_a(const data_subscriber_8_bit_ptr &subscriber) {
  port_a_subscribers_.erase(subscriber);
}
