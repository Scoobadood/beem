//
// Created by Dave Durbin on 3/1/2023.
//

#include "user_via.h"

#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>

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

void UserVia::set_pcr(uint8_t value) {
  pcr_ = value;
  spdlog::get("UserVIA")->info("Set PCR (0x{:02x})", value);
}