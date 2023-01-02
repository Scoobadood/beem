//
// Created by Dave Durbin on 30/12/2022.
//

#include "via.h"
#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>

SystemVia::SystemVia() {
  input_latching_ = false;
  ddra_ = 0x00;
  ddrb_ = 0x00;
  orb_ = 0x00;
  ora_ = 0x00;
  ira_ = 0x00;
  irb_ = 0x00;
  sound_chip_enabled_ = false;
  read_speech_enabled_ = false;
  write_speech_enabled_ = false;
  keyb_autoscan_enabled_ = false;
  caps_lock_led_ = false;
  shift_lock_led_ = false;
  c0_ = 0;
  c1_ = 0;
  ca1_ = 0;
  ca2_ = 0;
  cb1_ = 0;
  cb2_ = 0;

  try {
    auto logger = spdlog::basic_logger_mt("SystemVIA", "logs/system-via.txt", true);
    logger->set_pattern("[SystemVIA] [%^%l%$] %v");
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
}

void SystemVia::set_ddra(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set DDRA 0x{:0X}", value);
  ddra_ = value;
}

void SystemVia::set_ddrb(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set DDRB 0x{:0X}", value);
  ddrb_ = value;
}

void SystemVia::set_orb(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set ORB 0x{:0X}", value);
  orb_ = value;
  write_port_b();
}

void SystemVia::set_ora(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set ORA 0x{:0X}", value);
  ora_ = value;
  write_port_a();
}

uint8_t SystemVia::get_irb() {
  spdlog::get("SystemVIA")->info("Read IRB [0x{:0X}]", irb_);
  return irb_;
}

uint8_t SystemVia::get_ira() {
  spdlog::get("SystemVIA")->info("Read IRA [0x{:0X}]", ira_);
  if( !input_latching_) {
    ira_ = read_port_a();
  }
  return ira_;
}

  void SystemVia::write_port_a() {
  spdlog::warn("Not Implemented: SystemVIA::write_port_a()");
}

void SystemVia::write_port_b() {
  auto out_value = (orb_ & 0xf) & ddrb_;
  switch (out_value) {
    case 0:sound_chip_enabled_ = true;
      spdlog::get("SystemVIA")->info("Sound chip enabled");
      break;
    case 1:read_speech_enabled_ = true;
      spdlog::get("SystemVIA")->info("Read speech enabled");
      break;
    case 2:write_speech_enabled_ = true;
      spdlog::get("SystemVIA")->info("Write speech enabled");
      break;
    case 3:keyb_autoscan_enabled_ = false;
      spdlog::get("SystemVIA")->info("Keyboard autoscan disabled");
      break;
    case 4:
      c0_ = 0;
      spdlog::get("SystemVIA")->info("HW Scrolling C0=0");
      break;
    case 5:
      c1_ = 0;
      spdlog::get("SystemVIA")->info("HW Scrolling C1=0");
      break;
    case 6:
      caps_lock_led_ = true;
      spdlog::get("SystemVIA")->info("CAPS Lock LED on");
      break;
    case 7:
      shift_lock_led_ = true;
      spdlog::get("SystemVIA")->info("Shift Lock LED on");
      break;
    case 8:sound_chip_enabled_ = false;
      spdlog::get("SystemVIA")->info("Sound chip disabled");
      break;
    case 9:read_speech_enabled_ = false;
      spdlog::get("SystemVIA")->info("Read speech disabled");
      break;
    case 10:write_speech_enabled_ = false;
      spdlog::get("SystemVIA")->info("Write speech disabled");
      break;
    case 11:keyb_autoscan_enabled_ = true;
      spdlog::get("SystemVIA")->info("Keyboard autoscan enabled");
      break;
    case 12:c0_ = 1;
      spdlog::get("SystemVIA")->info("HW Scrolling C0=1");
      break;
    case 13:c1_ = 1;
      spdlog::get("SystemVIA")->info("HW Scrolling C1=1");
      break;
    case 14:caps_lock_led_ = false;
      spdlog::get("SystemVIA")->info("CAPS Lock LED off");
      break;
    case 15:shift_lock_led_ = false;
      spdlog::get("SystemVIA")->info("Shift Lock LED off");
      break;
  }
}


uint8_t read_port_a() {

}

/**
 * When reading from this address the top four bits are read:
 * bit 7:    Speech processor 'ready' signal
 * bit 6:    Speech processor 'interrupt' signal
 * bit 4-5:  joystick buttons (bit is zero when button pressed)
 * @return
 */
uint8_t SystemVia::read_port_b() {
  irb_ = 0x00;
  return irb_;
}

