//
// Created by Dave Durbin on 3/1/2023.
//

#include "user_via.h"

#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>

static const   std::string control_modes[] = {
    "negative edges active on input",
    "independent interrupt; input negative edge",
    "positive edges active on input",
    "independent interrupt; input positive edge",
    "handshake output mode",
    "pulse output mode",
    "low output",
    "high output"
};

UserVia::UserVia() {
  input_latching_ = false;
  ddra_ = 0x00;
  ddrb_ = 0x00;
  orb_ = 0x00;
  ora_ = 0x00;
  ira_ = 0x00;
  // no joystick buttons depressed
  irb_ = 0x30;
  /*
   * On power on: 6522 System VIA IER bits 0 to 6 will all be clear
   * (NB bit 7 is always returned as 1)
   */
  ier_ = 0x00;

  ifr_ = 0;

  try {
    auto logger = spdlog::basic_logger_mt("UserVIA", "logs/user-via.txt", true);
    spdlog::flush_every((std::chrono::seconds) 5);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
}



void UserVia::set_ddra(uint8_t value) {
  spdlog::get("UserVIA")->info("Set DDRA 0x{:02x}", value);
  ddra_ = value;
}

uint8_t UserVia::ddra() const {
  spdlog::get("UserVIA")->info("Get DDRA (0x{:02x})", ddra_);
  return ddra_;
}

void UserVia::set_ddrb(uint8_t value) {
  spdlog::get("UserVIA")->info("Set DDRB 0x{:02x}", value);
  ddrb_ = value;
}

uint8_t UserVia::ddrb() const {
  spdlog::get("UserVIA")->info("Get DDRB (0x{:02x})", ddrb_);
  return ddrb_;
}

void UserVia::set_orb(uint8_t value) {
  orb_ = value;
  write_port_b();
}

void UserVia::set_ora(uint8_t value) {
  spdlog::get("UserVIA")->info("Set ORA 0x{:02x} (ddra is {:02x})", value, ddra_);
  ora_ = (ddra_ & value);
  write_port_a();
}

uint8_t UserVia::irb() const {
  spdlog::get("UserVIA")->info("Read IRB (0x{:02X})", irb_);
  return irb_;
}

uint8_t UserVia::ira() const {
  spdlog::get("UserVIA")->info("Read IRA (0x{:02X})", ira_);
  // TODO Input latching
  return ira_;
}

void UserVia::write_port_a() {
  spdlog::warn("Not Implemented: UserVIA::write_port_a() with ddra==0x{:02x}", ddra_);
}

void UserVia::write_port_b() {
  auto out_value = (orb_ & 0xf) & ddrb_;
  spdlog::get("UserVIA")->warn("Set ORB 0x{:0X}", out_value);
}

uint8_t UserVia::read_port_a() const {
  spdlog::warn("Not Implemented: UserVIA::read_port_a()");
  return 0x00;
}

uint8_t UserVia::read_port_b() {
  spdlog::warn("Not Implemented: UserVIA::read_port_b()");
  return 0x00;
}

/*
 * The 6502 can set or clear selected bits in the interrupt enable register without affecting the other bits.
 * This is accomplished by writing to the IER.
 * If bit 7 of the byte written is a 0 then each 1 in bits 0–6 will
 * clear the corresponding bit in the IER. For each zero in bits 0–6, the corresponding bit will not be affected.
 * Selected bits can be SET in a similar manner. In this case, bit 7 of the written byte should be set to 1.
 * Each 1 in bits 0–6 will then SET the selected bit. A zero will cause the corresponding bit to remain unaffected.
 * The contents of the IER can be read by the 6502. Bit 7 is then always read as a logic 1.
 */
uint8_t UserVia::ier() const {
  spdlog::get("UserVIA")->info("Get IER (0x{:02X})", (ier_ | 0x80));
  return 0x80 | ier_;
}

void UserVia::set_ier(uint8_t value) {
  if (value & 0x80) {
    ier_ = ier_ & (~value);
  } else {
    ier_ = ier_ | value;
  }
  spdlog::get("UserVIA")->info("Set IER (0x{:02x}) | TIMER 1 {} | TIMER 2 {} | CB1 {} | CB2 {} | SHIFT_REG {} | CA1 {} | CA2 {} |", value,
                                 (ier_ & 0x40) ? "X" : " ",
                                 (ier_ & 0x20) ? "X" : " ",
                                 (ier_ & 0x10) ? "X" : " ",
                                 (ier_ & 0x08) ? "X" : " ",
                                 (ier_ & 0x04) ? "X" : " ",
                                 (ier_ & 0x02) ? "X" : " ",
                                 (ier_ & 0x01) ? "X" : " "
  );
}

uint8_t UserVia::ifr() const {
  spdlog::get("UserVIA")->info("Read IFR (0x{:02X}) | MASTER {} | TIMER 1 {} | TIMER 2 {} | CB1 (EOC) {} | CB2 (LPSTB) {} | SHIFT_REG {} | CA1 (VSYNC) {} | CA2 (KEYB) {} |",
                                 ifr_,
                                 (ifr_ & 0x80) ? "X" : " ",
                                 (ifr_ & 0x40) ? "X" : " ",
                                 (ifr_ & 0x20) ? "X" : " ",
                                 (ifr_ & 0x10) ? "X" : " ",
                                 (ifr_ & 0x08) ? "X" : " ",
                                 (ifr_ & 0x04) ? "X" : " ",
                                 (ifr_ & 0x02) ? "X" : " ",
                                 (ifr_ & 0x01) ? "X" : " "
  );

  return ifr_;
}

void UserVia::set_ifr(uint8_t data) {
  if (data & 0x80) {
    return;
  }
  ifr_ = (ifr_ ^ data);
  if ((ifr_ & 0x7f) == 0) {
    ifr_ ^= 0x80;
  }
  spdlog::get("UserVIA")->info(
      "Set IFR (0x{:02X}) | MASTER {} | TIMER 1 {} | TIMER 2 {} | CB1 (EOC) {} | CB2 (LPSTB) {} | SHIFT_REG {} | CA1 (VSYNC) {} | CA2 (KEYB) {} |",
      ifr_,
      (ifr_ & 0x80) ? "X" : " ",
      (ifr_ & 0x40) ? "X" : " ",
      (ifr_ & 0x20) ? "X" : " ",
      (ifr_ & 0x10) ? "X" : " ",
      (ifr_ & 0x08) ? "X" : " ",
      (ifr_ & 0x04) ? "X" : " ",
      (ifr_ & 0x02) ? "X" : " ",
      (ifr_ & 0x01) ? "X" : " "
  );
}

void UserVia::set_pcr(uint8_t value) {
  pcr_ = value;
  /*
 * System VIA, Peripheral Control Register ($FE4C) (aka 'PCR')
 *
 * bit 0    = CA1 interrupt control
 *            Writing to CA1 means "data taken"
 *            0 means negative active edge
 *            1 means positive active edge
 *
 * bits 1-3 = CA2 control mode
 *            CA2 signifies "data ready"
 *
 * bit 4    = CB1 interrupt control
 *            Writing to CB1 means "data taken"
 *            0 means negative active edge
 *            1 means positive active edge
 *
 * bits 5-7 = CB2 control mode
 *            CB2 signifies "data ready"
 *
 * control mode:
 *   000 = negative edges active on input
 *   001 = independent interrupt; input negative edge
 *   010 = positive edges active on input
 *   011 = independent interrupt; input positive edge
 *   100 = handshake output mode
 *   101 = pulse output mode
 *   110 = low output
 *   111 = high output
 *
 * The System VIA PCR initialises like so (See .setUpPage2):
 *       CA1 has negative active edge       (vertical sync)
 *       CA2 positive edges active on input (keyboard)
 *       CB1 has negative active edge       (end of analogue conversion)
 *       CB2 negative active edges on input (light pen strobe)
 */
  pcr_ = value;

  uint8_t ca2_control_mode = (value & 0x0e) >> 1;
  uint8_t cb2_control_mode = (value & 0xd0) >> 5;
  spdlog::get("UserVIA")->info("Set PCR (0x{:02x})  CA1: {}  CA2 Mode: {}  CB1: {}  CB2 Mode: {}",
                                 value,
                                 (value & 0x01) ? "positive active edge" : "negative active edge",
                                 control_modes[ca2_control_mode],
                                 (value & 0x10) ? "positive active edge" : "negative active edge",
                                 control_modes[cb2_control_mode]
  );
}

uint8_t UserVia::pcr() const {
  uint8_t ca2_control_mode = (pcr_ & 0x0e) >> 1;
  uint8_t cb2_control_mode = (pcr_ & 0xd0) >> 5;
  spdlog::get("UserVIA")->info(
      "Read PCR (0x{:02x})  CA1: {}  CA2 Mode: {}  CB1: {}  CB2 Mode: {}",
      pcr_,
      (pcr_ & 0x01) ? "positive active edge" : "negative active edge",
      control_modes[ca2_control_mode],
      (pcr_ & 0x10) ? "positive active edge" : "negative active edge",
      control_modes[cb2_control_mode]
  );
  return pcr_;
}



/*
;
; System VIA, Auxiliary Control Register ($FE4B) (aka 'ACR')
;
; bit 0:    PA latch enable
; bit 1:    PB latch enable
; bits 2-4: Shift register mode
; bit 5:    Timer 2 mode: 0=One-shot mode; 1=Pulse counting mode.
; bit 6:    Timer 1 mode: 0=One shot mode; 1=Free-run mode.
; bit 7:    Enable pulsing of System VIA output pin PB7.
;           When enabled, Timer 1 will set PB7 as follows:
;           In One-shot mode:
;               PB7 is cleared when Timer 1 started,
;               PB7 is set when Timer 1 one-shot mode times out.
;           In Free-run mode:
;               PB7 is inverted when Timer 1 times out.
;
; In the reset code (see .setUpPage2) this register is initialised to:
;
;   (a) disable the latches and the shift register,
;   (b) set Timer 2 as an interval timer,
;   (c) set Timer 1 as free-run mode (aka continuous interrupts).
;
; Otherwise this register is not used by the OS.
;
; See NAUG Section 22.4.8, Page 395.
;
*/
void UserVia::set_acr(uint8_t value) {
  uint32_t sr_mode = (value & 0x1c) >> 2;
  spdlog::get("UserVIA")->info(
      "Set ACR (0x{:02x}) | Pulse PB7 {} | TIMER 1 {} | TIMER 2 {} | SR Mode {} | PB Latch enable {} | PA Latch enable {} |",
      ifr_,
      (ifr_ & 0x80) ? "X" : " ",
      (ifr_ & 0x40) ? "Free run" : "One shot",
      (ifr_ & 0x20) ? "Pulse count" : "One shot",
      sr_mode,
      (ifr_ & 0x02) ? "X" : " ",
      (ifr_ & 0x01) ? "X" : " "
  );
  acr_ = value;
}