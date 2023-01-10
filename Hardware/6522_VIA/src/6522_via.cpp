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

Via::Via(uint16_t base_address) //
    : base_address_{base_address} //
    , ddra_{0} //
    , ddrb_{0} //
{
}

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
  switch (reg) {
    case IORB:spdlog::info("Read IRB");
      break;
    case IORA:spdlog::info("Read IRA");
      break;
    case DDRB:spdlog::info("Read DDRB ({:02x})", ddrb_);
      bus.set_data(ddrb_);
      break;
    case DDRA:spdlog::info("Read DDRA ({:02x})", ddra_);
      bus.set_data(ddra_);
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
    case IFR:spdlog::info("Read IFR");
      break;
    case IER:spdlog::info("Read IER");
      break;
    case IORA_NOH:spdlog::info("Read IRA_NOH");
      break;
    default:spdlog::error("Read Unknown register ({:02x})", reg);
      break;

  }
}

void Via::mmio_write(Bus &bus, uint8_t reg) {
  auto data = bus.get_data();
  switch (reg) {
    case IORB:spdlog::info("Wrote ({:02x}) to ORB", data);
      break;
    case IORA:spdlog::info("Wrote ({:02x}) to ORA", data);
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
    case IER:spdlog::info("Wrote ({:02x}) to IER", data);
      break;
    case IORA_NOH:spdlog::info("Wrote ({:02x}) to IRA_NOH", data);
      break;
    default:spdlog::error("Wrote ({:02x}) tou nknown register ({:02x})", data,reg);
      break;
  }
}

void Via::tick(Bus &bus) {
  check_mmio(bus);
}
