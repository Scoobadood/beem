//
// Created by Dave Durbin on 30/12/2022.
//

#include "memory.h"
#include "via.h"

#include <iterator>
#include <fstream>

#include <spdlog/spdlog-inl.h>


// Sheila 0xFE

// CRTC
const uint16_t SHEILA = 0xfe00;
const uint16_t CRTC_register_addr = SHEILA + 0x00;
const uint16_t CRTC_register_data = SHEILA + 0x01;
//&00–&07 6845 CRTC


//&08–&0F 6850 ACIA
//&10–&1F Serial ULA
//&20–&2F Video ULA
//&30–&3F 74LS161

//FE40-5F System 6522 VIA
//=======================
//     Write                              Read
//FE40 Output register B                  Input register B
//FE41 Output register A                  Input register A
//FE42 Data direction register B          Data direction register B
//FE43 Data direction register A          Data direction register A
//FE44 T1 low-order counter               T1 low-order latches
//FE45 T1 high-order counter              T1 high-order counter
//FE46 T1 low-order latches               T1 low-order latches
//FE47 T1 high-order latches              -
//FE48 T2 low-order latches               T2 low-order counter
//FE49 T2 high order counter              T2 high order counter
//FE4A Shift register                     Shift register
//FE4B Auxilary control register          Auxilary control register
//FE4C Peripheral control register        Peripheral control register
//FE4D Interrupt flag register            Interrupt flag register
//FE4E Interrupt enable register          Interrupt enable register
//FE4F Output register A, no handshake    Input register A, no handshake

const uint16_t SystemVIA_ORB                 = SHEILA + 0x40;
const uint16_t SystemVIA_IRB                 = SHEILA + 0x40;
const uint16_t SystemVIA_ORA                 = SHEILA + 0x41;
const uint16_t SystemVIA_IRA                 = SHEILA + 0x41;
const uint16_t SystemVIA_DDRB                = SHEILA + 0x42;
const uint16_t SystemVIA_DDRA                = SHEILA + 0x43;
const uint16_t SystemVIA_t1_low_order_cnt    = SHEILA + 0x44;
const uint16_t SystemVIA_t1_high_order_cnt   = SHEILA + 0x45;
const uint16_t SystemVIA_t1_low_order_latch  = SHEILA + 0x46;
const uint16_t SystemVIA_t1_high_order_latch = SHEILA + 0x47;
const uint16_t SystemVIA_t2_low_order_latch  = SHEILA + 0x48;
const uint16_t SystemVIA_t2_high_order_cnt   = SHEILA + 0x49;
const uint16_t SystemVIA_shift_reg           = SHEILA + 0x4A;
const uint16_t SystemVIA_aux_control_reg     = SHEILA + 0x4B;
const uint16_t SystemVIA_peripheral_ctl_reg  = SHEILA + 0x4C;
const uint16_t SystemVIA_int_flag_reg        = SHEILA + 0x4D;
const uint16_t SystemVIA_int_enable_reg      = SHEILA + 0x4E;
const uint16_t SystemVIA_ORA_NoHshk          = SHEILA + 0x4F;



//&60–&7F 6522 VIA
//&80–&9F 8271 FDC
//&A0–&BF 68B54 ADLC
//&C0–&DF uPD7002
//&E0–&FF Tube ULA



Memory::Memory(uint16_t sz) {
  size_ = sz;
  memory_.resize(sz, 0);
}

Memory::Memory(std::ifstream &f) {
  using namespace std;
  memory_ = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
  size_ = memory_.size();
}

uint8_t Memory::at(uint16_t addr) const {
  assert (addr < size_);
  return memory_.at(addr);
}

void Memory::set(uint16_t addr, uint8_t arg) {
  if (addr >= 0xc000)
    handle_rom_writes(addr, arg);
  else
    memory_.at(addr) = arg;
}

void Memory::handle_rom_writes(uint16_t addr, uint8_t arg) {
  switch( addr) {
    case CRTC_register_addr:
      spdlog::info("Select CRTC register 0x{:0X}", arg);
      break;
    case CRTC_register_data:
      spdlog::info("Write CRTC data 0x{:0X}", arg);
      break;
    case SystemVIA_DDRB:
      system_via_.set_ddrb(arg);
      break;
    case SystemVIA_DDRA:
      system_via_.set_ddra(arg);
      break;
    case SystemVIA_ORB:
      system_via_.set_orb(arg);
      break;
    case SystemVIA_ORA:
      system_via_.set_ora(arg);
      break;
    case SystemVIA_ORA_NoHshk:
      spdlog::warn("Not impl: Write 0x{:0X} to port A (no handshake)", arg);
      break;

    default:
      spdlog::info("write {:0X} to 0x{:0X}", arg, addr);
      break;
  }
}

void Memory::insert(uint16_t offset, std::vector<uint8_t> &data) {
  memory_.insert(memory_.begin() + offset, data.begin(), data.end());
}



