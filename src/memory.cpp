//
// Created by Dave Durbin on 30/12/2022.
//

#include "memory.h"
#include "system_via.h"
#include "m6502.h"

#include <iterator>
#include <fstream>

#include <spdlog/spdlog-inl.h>

const uint32_t STACK_BASE = 0x100;


// Sheila 0xFE

// CRTC
const uint16_t SHEILA = 0xfe00;
const uint16_t CRTC_register_addr = SHEILA + 0x00;
const uint16_t CRTC_register_data = SHEILA + 0x01;
//&00–&07 6845 CRTC


// 6850 ACIA - Async Comms Interface Adaptor 0xfe08–0xfe0f
const uint16_t ACIA_CTL = SHEILA + 0x08; // Control Register W/O
const uint16_t ACIA_STATUS = SHEILA + 0x08; // Status Register R/O
const uint16_t ACIA_TDR = SHEILA + 0x09; // Transmit Data Register W/O
const uint16_t ACIA_RDR = SHEILA + 0x09; // Receive Data Register R/O

//Serial ULA - uncomitted Logic Array 0xfe10–0xfe1f
const uint16_t S_ULA_CTL = SHEILA + 0x10; // Serial ULA Ctl. WO

//&20–&2F Video ULA
//&30–&3F 74LS161
//&80–&9F 8271 FDC
//&A0–&BF 68B54 ADLC
//&C0–&DF uPD7002
//&E0–&FF Tube ULA

// Sytem VIA : 0xfe40 - 0xfe4f
const uint16_t SystemVIA_ORB = SHEILA + 0x40;
const uint16_t SystemVIA_IRB = SHEILA + 0x40;
const uint16_t SystemVIA_ORA = SHEILA + 0x41;
const uint16_t SystemVIA_IRA = SHEILA + 0x41;
const uint16_t SystemVIA_DDRB = SHEILA + 0x42;
const uint16_t SystemVIA_DDRA = SHEILA + 0x43;
const uint16_t SystemVIA_t1_low_order_cnt = SHEILA + 0x44;
const uint16_t SystemVIA_t1_high_order_cnt = SHEILA + 0x45;
const uint16_t SystemVIA_t1_low_order_latch = SHEILA + 0x46;
const uint16_t SystemVIA_t1_high_order_latch = SHEILA + 0x47;
const uint16_t SystemVIA_t2_low_order_latch = SHEILA + 0x48;
const uint16_t SystemVIA_t2_high_order_cnt = SHEILA + 0x49;
const uint16_t SystemVIA_shift_reg = SHEILA + 0x4A;
const uint16_t SystemVIA_aux_control_reg = SHEILA + 0x4B;
const uint16_t SystemVIA_peripheral_ctl_reg = SHEILA + 0x4C;
const uint16_t SystemVIA_int_flag_reg = SHEILA + 0x4D;
const uint16_t SystemVIA_int_enable_reg = SHEILA + 0x4E;
const uint16_t SystemVIA_ORA_NoHshk = SHEILA + 0x4F;
const uint16_t SystemVIA_IRA_NoHshk = SHEILA + 0x4F;

// User VIA : 0xfe60 - 0xfe6f
const uint16_t UserVIA_ORB = SHEILA + 0x60;
const uint16_t UserVIA_IRB = SHEILA + 0x60;
const uint16_t UserVIA_ORA = SHEILA + 0x61;
const uint16_t UserVIA_IRA = SHEILA + 0x61;
const uint16_t UserVIA_DDRB = SHEILA + 0x62;
const uint16_t UserVIA_DDRA = SHEILA + 0x63;
const uint16_t UserVIA_t1_low_order_cnt = SHEILA + 0x64;
const uint16_t UserVIA_t1_high_order_cnt = SHEILA + 0x65;
const uint16_t UserVIA_t1_low_order_latch = SHEILA + 0x66;
const uint16_t UserVIA_t1_high_order_latch = SHEILA + 0x67;
const uint16_t UserVIA_t2_low_order_latch = SHEILA + 0x68;
const uint16_t UserVIA_t2_high_order_cnt = SHEILA + 0x69;
const uint16_t UserVIA_shift_reg = SHEILA + 0x6A;
const uint16_t UserVIA_aux_control_reg = SHEILA + 0x6B;
const uint16_t UserVIA_peripheral_ctl_reg = SHEILA + 0x6C;
const uint16_t UserVIA_int_flag_reg = SHEILA + 0x6D;
const uint16_t UserVIA_int_enable_reg = SHEILA + 0x6E;
const uint16_t UserVIA_ORA_NoHshk = SHEILA + 0x6F;
const uint16_t UserVIA_IRA_NoHshk = SHEILA + 0x6F;

Memory::Memory(uint32_t sz) {
  size_ = sz;
  memory_.resize(sz, 0);
  system_via_ = nullptr;
}

Memory::Memory(std::ifstream &f) {
  using namespace std;
  memory_ = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
  size_ = memory_.size();
  system_via_ = nullptr;
}

void Memory::set_system_via(SystemVia *system_via) {
  system_via_ = system_via;
}

void Memory::set_user_via(UserVia *user_via) {
  user_via_ = user_via;
}

void Memory::set_acia(Acia *acia) {
  acia_ = acia;
}

void Memory::push_stack(M6502 &cpu, uint8_t arg) {
  set(STACK_BASE + cpu.sp(), arg & 0xff);
  cpu.set_sp((cpu.sp() - 1) & 0xff);
}

uint8_t Memory::pop_stack(M6502 &cpu) const {
  cpu.set_sp((cpu.sp() + 1) & 0xff);
  return at(STACK_BASE + cpu.sp()) & 0xff;
}


uint8_t Memory::at(uint16_t addr) const {
  assert (addr < size_);

  if (addr >= 0xfc00 && addr <= 0xfeff)
    return handle_mmio_reads(addr);

  return memory_.at(addr);
}

void Memory::set(uint16_t addr, uint8_t arg) {
  assert (addr < size_);
  if (addr >= 0xfc00 && addr <= 0xfeff)
    handle_mmio_writes(addr, arg);
  else
    memory_.at(addr) = arg;
}

uint8_t Memory::handle_mmio_reads(uint16_t addr) const {
//  if (system_via_ == nullptr) {
//    spdlog::warn("No SystemVIA!");
//  }
//  if (user_via_ == nullptr) {
//    spdlog::warn("No UserVIA!");
//  }
//  if (acia_ == nullptr) {
//    spdlog::warn("No Acia!");
//  }
//
//  switch (addr) {
//    case SystemVIA_int_enable_reg: {
//      auto ier = system_via_->ier();
//      return ier;
//    }
//
//    case ACIA_STATUS: return acia_->status();
//    case ACIA_RDR: return acia_->rdr();
//
//    case SystemVIA_DDRA:return system_via_->ddra();
//    case SystemVIA_DDRB:return system_via_->ddrb();
//    case SystemVIA_IRB:return system_via_->irb();
//    case SystemVIA_IRA_NoHshk:return system_via_->ira();
//    case SystemVIA_peripheral_ctl_reg: return system_via_->pcr();
//    case SystemVIA_int_flag_reg: return system_via_->ifr();
//
//    case UserVIA_DDRA:return user_via_->ddra();
//    case UserVIA_DDRB:return user_via_->ddrb();
//    case UserVIA_IRA_NoHshk:return user_via_->ira();
//    case UserVIA_IRB:return user_via_->irb();
//    case UserVIA_peripheral_ctl_reg: return user_via_->pcr();
//
//
//    default:spdlog::info("Read from ROM address 0x{:04x} not implemented", addr);
//      return 0x00;
//  }
}

void Memory::handle_mmio_writes(uint16_t addr, uint8_t arg) {
//  if (system_via_ == nullptr) {
//    spdlog::warn("No SystemVIA!");
//  }
//  if (user_via_ == nullptr) {
//    spdlog::warn("No UserVIA!");
//  }
//  if (acia_ == nullptr) {
//    spdlog::warn("No Acia!");
//  }
//
//  switch (addr) {
//    case CRTC_register_addr:spdlog::info("Select CRTC register 0x{:0X}", arg);
//      break;
//    case CRTC_register_data:spdlog::info("Write CRTC data 0x{:0X}", arg);
//      break;
//    case ACIA_CTL: acia_->set_ctl(arg);
//      break;
//    case ACIA_TDR: acia_->set_tdr(arg);
//      break;
//    case S_ULA_CTL: acia_->set_ula_ctl(arg);
//      break;
//
//    case SystemVIA_DDRB:system_via_->set_ddrb(arg);
//      break;
//    case SystemVIA_DDRA:system_via_->set_ddra(arg);
//      break;
//    case SystemVIA_ORB:system_via_->set_orb(arg);
//      break;
//    case SystemVIA_int_flag_reg: system_via_->set_ifr(arg);
//      break;
//    case SystemVIA_int_enable_reg: system_via_->set_ier(arg);
//      break;
//    case SystemVIA_peripheral_ctl_reg: system_via_->set_pcr(arg);
//      break;
//    case SystemVIA_aux_control_reg: system_via_->set_acr(arg);
//      break;
//    case SystemVIA_ORA:
//    case SystemVIA_ORA_NoHshk:system_via_->set_ora(arg);
//      break;
//    case SystemVIA_t1_high_order_latch:
//      system_via_->set_T1_latch_high(arg);
//      break;
//    case SystemVIA_t1_high_order_cnt:
//      system_via_->set_T1_counter_high(arg);
//      break;
//    case SystemVIA_t1_low_order_latch:
//      system_via_->set_T1_latch_low(arg);
//      break;
//    case SystemVIA_t1_low_order_cnt:
//      system_via_->set_T1_counter_low(arg);
//      break;
//
//    case UserVIA_DDRB:user_via_->set_ddrb(arg);
//      break;
//    case UserVIA_DDRA:user_via_->set_ddra(arg);
//      break;
//    case UserVIA_ORB:user_via_->set_orb(arg);
//      break;
//    case UserVIA_ORA:
//    case UserVIA_ORA_NoHshk:user_via_->set_ora(arg);
//      break;
//    case UserVIA_int_enable_reg:user_via_->set_ier(arg);
//      break;
//    case UserVIA_int_flag_reg: system_via_->set_ifr(arg);
//      break;
//    case UserVIA_peripheral_ctl_reg: system_via_->set_pcr(arg);
//      break;
//    case UserVIA_aux_control_reg: system_via_->set_acr(arg);
//      break;
//
//    default:spdlog::info("write {:0X} to 0x{:0X}", arg, addr);
//      break;
//  }
}

void Memory::insert(uint16_t offset, std::vector<uint8_t> &data) {
  memory_.insert(memory_.begin() + offset, data.begin(), data.end());
}



