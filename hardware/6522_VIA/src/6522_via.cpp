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
    , ddrb_{0} //
    , irb_{0} //
    , orb_{0} //
    , cb1_{0} //
    , cb2_{0} //
    , ier_{0} //
    , ifr_{0} //
    , acr_{0} //
    , pcr_{0} //
{}

void Via::check_mmio(Bus &bus) {
  auto addr = bus.get_address();
  if (addr < base_address_) {
    return;
  }

  addr -= base_address_;
  if (addr > 0x0F) {
    return;
  }

  if (bus.tst_RW()) {
    mmio_read(bus, addr);
  } else {
    mmio_write(bus, addr);
  }
}

void Via::mmio_read(Bus &bus, uint8_t reg) {
  uint8_t data = bus.get_data();
  switch (reg) {
    case IORB:data = irb_ & ~ddrb_;
      spdlog::info("Read ({:02x}) from IRB", data);
      break;
    case IORA:
    case IORA_NOH:data = read_port_a();
      spdlog::info("Read ({:02x}) from IRA{}", data, (reg == IORA ? "" : "_NOH"));
      break;

    case DDRB:data = ddrb_;
      spdlog::info("Read ({:02x}) from DDRB", data);
      break;
    case DDRA:data = ddra_;
      spdlog::info("Read ({:02x}) from DDRA", data);
      break;
    case T1C_L:spdlog::info("Read T1C_L");
      break;
    case T1C_H:spdlog::info("Read T1C_H");
      break;
    case T1L_L:spdlog::info("Read T1L_L");
      break;
    case T1L_H:spdlog::info("Read T1L_H");
      break;
    case T2C_L:spdlog::info("Read T2C_L");
      break;
    case T2C_H:spdlog::info("Read T2C_H");
      break;
    case SR:spdlog::info("Read SR");
      break;
    case ACR:spdlog::info("Read ACR");
      break;
    case PCR:spdlog::info("Read PCR");
      break;
    case IFR:
      if (ifr_ & 0x7f) data = ifr_ | 0x80;
      else data = 0;
      spdlog::info("Read ({:02x}) from IFR");
      break;
    case IER:
      /**
       * Reading:
       * Bits 0-6 are read as expected.
       * Bit 7 is always set when read.
       */
      data = (ier_ | 0x80);
      spdlog::info("Read ({:02x}) from IER", data);
      break;
    default:spdlog::error("Read Unknown register ({:02x})", reg);
      break;
  }
  bus.set_data(data);
}

uint8_t Via::read_port_a() {
  auto data_fetched = 0;
  uint8_t out;
  for (auto &provider: port_a_providers_) {
    if (provider->has_data()) {
      out = (ira_ & ~ddra_ & provider->data()) | (ora_ & ddra_);
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

    notify_subscribers(port_a_subscribers_, pa, ddra_);
  }
}

void Via::write_port_b(uint8_t data) {
  orb_ = data;
  if (ddrb_) {
    uint8_t pb = (orb_ & ddrb_) | ~ddrb_;

    if (acr_ & 0x80) {
      // TODO: Handle pulsed output from timer
      /**
       * Timer 1 free-run mode
       * In the free-running mode, PB7 is inverted and the interrupt flag is set
       * each time the counter has decremented to zero. The contents of the 16 bit latch are then transferred to
       * the counter, which decrements to zero again and so on.
       * This produces a true square wave of variable frequency on the PB7 output.
       */
    }
    notify_subscribers(port_b_subscribers_, pb, ddrb_);
  }
}

void Via::write_irq_enable(uint8_t data) {
  if (data & 0x80) {
    ier_ |= data;
  } else {
    ier_ &= ~(data | 0x80);
  }
  spdlog::info("VIA@{:04X}: set IER (0x{:02x}) T1:{} T2:{} CB1:{} CB2:{} SR:{} CA1:{} CA2:{}",
               base_address_, data,
               TST_T1(ier_) ? "1" : "0",
               TST_T2(ier_) ? "1" : "0",
               TST_CB1(ier_) ? "1" : "0",
               TST_CB2(ier_) ? "1" : "0",
               TST_SR(ier_) ? "1" : "0",
               TST_CA1(ier_) ? "1" : "0",
               TST_CA2(ier_) ? "1" : "0"
  );
}

void Via::mmio_write(Bus &bus, uint8_t reg) {
  auto data = bus.get_data();
  switch (reg) {
    case IORB:spdlog::info("Writing ({:02x}) to ORB", data);
      write_port_b(data);
      break;

    case IORA:
    case IORA_NOH:spdlog::info("Writing ({:02x}) to ORA{}", data, (reg == IORA ? "" : "_NOH"));
      write_port_a(data);
      break;

    case DDRB:spdlog::info("Wrote ({:02x}) to DDRB", data);
      ddrb_ = data;
      break;
    case DDRA:spdlog::info("Wrote ({:02x}) to DDRA", data);
      ddra_ = data;
      break;
    case T1C_L:spdlog::info("Wrote ({:02x}) to T1C_L", data);
      break;
    case T1C_H:spdlog::info("Wrote ({:02x}) to T1C_H", data);
      break;
    case T1L_L:spdlog::info("Wrote ({:02x}) to T1L_L", data);
      break;
    case T1L_H:spdlog::info("Wrote ({:02x}) to T1L_H", data);
      break;
    case T2C_L:spdlog::info("Wrote ({:02x}) to T2C_L", data);
      break;
    case T2C_H:spdlog::info("Wrote ({:02x}) to T2C_H", data);
      break;
    case SR:spdlog::info("Wrote ({:02x}) to SR", data);
      break;
    case ACR:spdlog::info("Wrote ({:02x}) to ACR", data);
      break;
    case PCR:spdlog::info("Wrote ({:02x}) to PCR", data);
      break;
    case IFR:spdlog::info("Wrote ({:02x}) to IFR", data);
      break;
    case IER:write_irq_enable(data);
      break;
    default:spdlog::error("Wrote ({:02x}) to unknown register ({:02x})", data, reg);
      break;
  }
}

void Via::tick(Bus &bus) {
  check_mmio(bus);
}

void Via::provide_port_a(data_provider_8_bit_ptr provider) {
  port_a_providers_.emplace(provider);
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
