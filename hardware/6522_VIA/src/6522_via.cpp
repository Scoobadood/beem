#include "6522_via.h"
#include "bus.h"

#include <spdlog/spdlog-inl.h>

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
{}

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
      spdlog::info("VIA@{:04X}: Read ({:02x}) from {} IRB",
                   base_address_, data,
                   PB_LATCHED(acr_) ? "(latched)" : "");
      break;

    case IORA:
    case IORA_NOH:
      if (PA_LATCHED(acr_)) {
        data = pa_latch_;
      } else {
        data = read_port_a();

      }
      spdlog::info("VIA@{:04X}: Read ({:02x}) from {} IRA{}",
                   base_address_, data,
                   PA_LATCHED(acr_) ? "(latched)" : "",
                   (reg == IORA ? "" : "_NOH"));
      break;

    case DDRB:
      data = ddrb_;
      spdlog::info("Read ({:02x}) from DDRB", data);
      break;

    case DDRA:
      data = ddra_;
      spdlog::info("Read ({:02x}) from DDRA", data);
      break;

    case T1C_L:
      spdlog::info("Read T1C_L");
      break;
    case T1C_H:
      spdlog::info("Read T1C_H");
      break;
    case T1L_L:
      spdlog::info("Read T1L_L");
      break;
    case T1L_H:
      spdlog::info("Read T1L_H");
      break;
    case T2C_L:
      spdlog::info("Read T2C_L");
      break;
    case T2C_H:
      spdlog::info("Read T2C_H");
      break;
    case SR:
      spdlog::info("Read SR");
      break;

    case ACR:
      data = acr_;
      spdlog::info("VIA@{:04X}: Read ({:02x}) ACR", base_address_, acr_);
      break;

    case PCR:
      data = pcr_;
      spdlog::info("VIA@{:04X}: Read ({:02x}) from PCR", base_address_, pcr_);
      break;

    case IFR:
      if (ifr_ & 0x7f) data = ifr_ | 0x80;
      else data = 0;
      spdlog::info("VIA@{:04X}: Read ({:02x}) from IFR", base_address_, data);
      break;

    case IER:
      /**
       * Reading:
       * Bits 0-6 are read as expected.
       * Bit 7 is always set when read.
       */
      data = (ier_ | 0x80);
      spdlog::info("VIA@{:04X}: Read ({:02x}) from IER", base_address_, data);
      spdlog::info("VIA@{:04X}: {} {} {} {} {} {} {}",
                   base_address_,
                   TST_T1(ier_) ? "T1 enabled" : "T1 disabled",
                   TST_T2(ier_) ? "T2 enabled" : "T2 disabled",
                   TST_CB1(ier_) ? "CB1 enabled" : "CB1 disabled",
                   TST_CB2(ier_) ? "CB2 enabled" : "CB2 disabled",
                   TST_SR(ier_) ? "SR enabled" : "SR disabled",
                   TST_CA1(ier_) ? "CA1 enabled" : "CA1 disabled",
                   TST_CA2(ier_) ? "CA2 enabled" : "CA2 disabled"
      );
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
  for (auto &provider: port_a_providers_) {
    if (provider->has_data()) {
      out = (~ddra_ & provider->data()) | (ora_ & ddra_);
      ++data_fetched;
    }
  }
  if (data_fetched == 0) {
    out = (ora_ | ~ddra_) & ira_;
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
  for (auto &provider: port_b_providers_) {
    if (provider->has_data()) {
      out = (~ddrb_ & provider->data()) | (orb_ & ddrb_);
      ++data_fetched;
    }
  }
  if (data_fetched == 0) {
    out = (orb_ | ~ddrb_) & irb_;
  }
  if (data_fetched > 1) {
    spdlog::error("VIA${:04x}: Multiple data providers read from PortB", base_address_);
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
  spdlog::info("VIA@{:04X}: Writing ({:02x}) to IER", base_address_, data);
  auto old_ier = ier_;
  if (data & 0x80) {
    ier_ |= data;
  } else {
    ier_ &= ~(data | 0x80);
  }

  // FIXME: Only report CA2 for debug purposes
  spdlog::info("VIA@{:04X}: write to IER. {}",
               base_address_,
               ((ier_ ^ old_ier) & IRQ_CA2) ? (TST_CA2(ier_) ? "CA2 enabled" : "CA2 disabled") : "CA2 unchanged"
  );
}

void Via::write_acr(uint8_t data) {
  spdlog::info("Writing ({:02x}) to ACR", data);

  acr_ = data;

  spdlog::info("VIA@{:04X}: PA_L {}",
               base_address_, PA_LATCHED(acr_) ? "enabled" : "disabled");
  spdlog::info("          PB_L {}", PB_LATCHED(acr_) ? "enabled" : "disabled");
  spdlog::info("          T1 {}", ACR_T1_CTL(acr_) ? "Continuous" : "One shot");
  spdlog::info("          T2 {}", ACR_T2_CTL(acr_) ? "Count down" : "One shot");
  spdlog::info("          PB7 {}", ACR_T1_PB7(acr_) ? "Enabled" : "Disabled");
  switch ((acr_ >> 2) & 0x7) {
    case 0:
      spdlog::info("          SR Disabled");
      break;
    case 1:
      spdlog::info("          SR Shift in T2");
      break;
    case 2:
      spdlog::info("          SR Shift in 1MHz");
      break;
    case 3:
      spdlog::info("          SR Shift in Ext Clk");
      break;
    case 4:
      spdlog::info("          SR Shift out Free running T2");
      break;
    case 5:
      spdlog::info("          SR Shift out T2");
      break;
    case 6:
      spdlog::info("          SR Shift out 1MHz");
      break;
    case 7:
      spdlog::info("          SR Shift out Ext Clk");
      break;
  }
}

void Via::write_pcr(uint8_t data) {
  spdlog::info("Writing ({:02x}) to PCR", data);
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

  spdlog::info("VIA@{:04X}: CA1:{} active edge", base_address_, cb1_pos_active_edge_ ? "positive" : "negative");
  spdlog::info("          CB1:{} active edge", cb1_pos_active_edge_ ? "positive" : "negative");
  spdlog::info("          CA2_CTL1:{}", ca2_ctl_);
  spdlog::info("          CB2_CTL1:{}", cb2_ctl_);
}

void Via::mmio_write(const std::shared_ptr<Bus> &bus, uint8_t reg) {
  auto data = bus->get_data();
  switch (reg) {
    case IORB:
      spdlog::info("Writing ({:02x}) to ORB", data);
      write_port_b(data);
      break;

    case IORA:
    case IORA_NOH:
      spdlog::info("Writing ({:02x}) to ORA{}", data, (reg == IORA ? "" : "_NOH"));
      write_port_a(data);
      break;

    case DDRB:
      spdlog::info("Writing ({:02x}) to DDRB", data);
      ddrb_ = data;
      break;
    case DDRA:
      spdlog::info("Writing ({:02x}) to DDRA", data);
      ddra_ = data;
      break;

    case T1C_L:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to T1C_L", base_address_, data);
      timer1_latch_ = (timer1_latch_ & 0xff00) | data;
      break;

    case T1C_H:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to T1C_H", base_address_, data);
      timer1_latch_ = (timer1_latch_ & 0xff) | (data << 8);
      timer1_count_ = timer1_latch_;
      if (ACR_T1_PB7(acr_)) pb7_ = 0;
      clear_irq(IRQ_T1);
      break;

    case T1L_L:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to T1L_L", base_address_, data);
      timer1_latch_ = (timer1_latch_ & 0xff00) | data;
      break;

    case T1L_H:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to T1L_H", base_address_, data);
      timer1_latch_ = (timer1_latch_ & 0xff) | (data << 8);
      break;

    case T2C_L:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to T2C_L", base_address_, data);
      timer2_latch_ = data;
      break;

    case T2C_H:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to T2C_H", base_address_, data);
      timer2_count_ = (data << 8) | timer2_latch_;
      clear_irq(IRQ_T2);
      break;

    case SR:
      spdlog::info("Wrote ({:02x}) to SR", data);
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
    case IFR:
      spdlog::info("VIA@{:04X}: Writing ({:02x}) to IFR", base_address_, data);
      {
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
      spdlog::error("Wrote ({:02x}) to unknown register ({:02x})", data, reg);
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
  if (state == prev_ca1_) return;
  prev_ca1_ = ca1_;
  ca1_ = state;

  if (ca1_ == (pcr_ & PCR_CA1_IRQ_CTL)) {
    // CA1 went active. Generate IRQ and latch data if enabled
    spdlog::info("VIA${:04x}: CA1 went active", base_address_);

    if (acr_ & ACR_PA_LATCH)
      pa_latch_ = read_port_a();

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
  if (state == prev_cb1_) return;
  prev_cb1_ = cb1_;
  cb1_ = state;

  if (cb1_ == (pcr_ & PCR_CB1_IRQ_CTL)) {
    // CB1 went active. Generate IRQ and latch data if enabled
    spdlog::info("VIA${:04x}: CB1 went active", base_address_);
    if (acr_ & ACR_PB_LATCH)
      pb_latch_ = read_port_b();

    raise_irq(IRQ_CB1);
  }
}

void Via::raise_irq(uint8_t irq) {
  if (TST_FLG(ifr_, irq)) return;
  std::string irq_name;
  switch (irq) {
    case 0x01:
      irq_name = "CA2";
      break;
    case 0x02:
      irq_name = "CA1";
      break;
    case 0x04:
      irq_name = "SR";
      break;
    case 0x08:
      irq_name = "CB2";
      break;
    case 0x10:
      irq_name = "CB1";
      break;
    case 0x20:
      irq_name = "T2";
      break;
    case 0x40:
      irq_name = "T1";
      break;
    default:
      irq_name = fmt::format("?? ({})", irq);
      break;
  }
  // FIXME: Remove this debugging for CA2 keyboard handling
  if (irq == IRQ_CA2) {
    spdlog::info("VIA@{:04x}: IRQ_CA2 was raised", base_address_, irq_name);
  }
  ifr_ |= (irq | IRQ_IRQ);
}

bool Via::has_irq() const {
  bool has = ((ifr_ & ier_ & 0x7f) != 0);
  spdlog::info("VIA@{:04x}: CPU polled to check for IRQs. {} {} (ifr: {:02x}) (ier: {:02x})",
               base_address_,
               (ifr_ != 0) ? "Interrupts present" : "No interrupts",
               has ? "and enabled" : "but disabled",
               ifr_, ier_);

  return has;
}

void Via::clear_irq(uint8_t irq) {
  // FIXME: Debugging added for keyboard interrupt handling. To remove
  if (irq == IRQ_CA2) {
    spdlog::info("VIA@{:04x}: IRQ_CA2 was cleared", base_address_);
  }
  if (!TST_FLG(ifr_, irq)) return;
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
  bool ca2_changed = false;
  bool ca2_low = false;
  for (const auto &provider: ca2_providers_) {
    if (provider->has_data()) {
      ca2_changed = true;
      auto data = provider->data();
      if (data == 0x00) {
        ca2_low = true;
        raise_irq(IRQ_CA2);
      }
      return;
    }
  }
  // Filter out System VIA
  if (base_address_ == 0xfe40) {
    spdlog::info("VIA@{:04x}: Checking CA2 from keyb", base_address_);
    spdlog::info("        : CA2 {} {}", ca2_changed ? "went" : "is still", ca2_low ? "low" : "high");
  }
}

void Via::tick(const std::shared_ptr<Bus> &bus) {
  check_timers();

  check_ca2();

  check_mmio(bus);
}

void Via::provide_port_a(data_provider_8_bit_ptr provider) {
  port_a_providers_.emplace(provider);
}

void Via::provide_port_b(data_provider_8_bit_ptr provider) {
  port_b_providers_.emplace(provider);
}

void Via::provide_ca2(data_provider_8_bit_ptr provider) {
  ca2_providers_.emplace(provider);
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
