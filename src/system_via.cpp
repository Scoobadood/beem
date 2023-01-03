//
// Created by Dave Durbin on 30/12/2022.
//

#include "system_via.h"
#include "keyboard.h"
#include "sound_76489.h"
#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>



SystemVia::SystemVia(Keyboard * keyboard, SoundChip * sound_chip) {
  input_latching_ = false;
  ddra_ = 0x00;
  ddrb_ = 0x00;
  orb_ = 0x00;
  ora_ = 0x00;
  ira_ = 0x00;
  // no joystick buttons depressed
  irb_ = 0x30;
  read_speech_enabled_ = false;
  write_speech_enabled_ = false;
  keyb_autoscan_enabled_ = false;
  caps_lock_led_ = false;
  shift_lock_led_ = false;
  c0_ = 0;
  c1_ = 0;
  /*
   * On power on: 6522 System VIA IER bits 0 to 6 will all be clear
   * (NB bit 7 is always returned as 1)
   */
  ier_ = 0x00;

  ifr_ = 0;

  keyboard_ = keyboard;
  sound_chip_ = sound_chip;

  try {
    auto logger = spdlog::basic_logger_mt("SystemVIA", "logs/system-via.txt", true);
    spdlog::flush_every((std::chrono::seconds) 5);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
}

void SystemVia::set_ddra(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set DDRA 0x{:02x}", value);
  ddra_ = value;
}

uint8_t SystemVia::ddra() const {
  spdlog::get("SystemVIA")->info("Get DDRA (0x{:02x})", ddra_);
  return ddra_;
}

void SystemVia::set_ddrb(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set DDRB 0x{:02x}", value);
  ddrb_ = value;
}

uint8_t SystemVia::ddrb() const {
  spdlog::get("SystemVIA")->info("Get DDRB (0x{:02x})", ddrb_);
  return ddrb_;
}

void SystemVia::set_orb(uint8_t value) {
  orb_ = value;
  write_port_b();
}

void SystemVia::set_ora(uint8_t value) {
  spdlog::get("SystemVIA")->info("Set ORA 0x{:02x} (ddra is {:02x})", value, ddra_);
  ora_ = (ddra_ & value);
  write_port_a();
}

uint8_t SystemVia::irb() const {
  spdlog::get("SystemVIA")->info("Read IRB (0x{:02X})", irb_);
  return irb_;
}

uint8_t SystemVia::ira() const {
  spdlog::get("SystemVIA")->info("Read IRA (0x{:02X})", ira_);
  // TODO Input latching
  return ira_;
}

void SystemVia::write_port_a() {
  if( ddra_ == 0x7f) {
    // Keyboard probing
    auto key_code = ora_ & 0x7f;
    if( keyboard_->key_pressed(key_code) ) {
      ira_ = 0x80 | key_code;
      spdlog::get("SystemVIA")->info("Probe key 0x{:02x} (pushed)", key_code);
    } else {
      ira_ = 0x00;
      spdlog::get("SystemVIA")->info("Probe key 0x{:02x} (not pushed)", key_code);
    }
    return;
  } else if ( ddra_ == 0xff) {
    spdlog::get("SystemVIA")->info("Poking the sound chip");
  } else {
    spdlog::warn("Not Implemented: SystemVIA::write_port_a() with ddra==0x{:02x}", ddra_);
  }
}

void SystemVia::write_port_b() {
  auto out_value = (orb_ & 0xf) & ddrb_;
  switch (out_value) {
    case 0:
      spdlog::get("SystemVIA")->info("Enabled sound chip");
      sound_chip_->enable();
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
    case 4:c0_ = 0;
      spdlog::get("SystemVIA")->info("HW Scrolling C0=0");
      break;
    case 5:c1_ = 0;
      spdlog::get("SystemVIA")->info("HW Scrolling C1=0");
      break;
    case 6:caps_lock_led_ = true;
      spdlog::get("SystemVIA")->info("CAPS Lock LED on");
      break;
    case 7:shift_lock_led_ = true;
      spdlog::get("SystemVIA")->info("Shift Lock LED on");
      break;
    case 8:
      spdlog::get("SystemVIA")->info("Disabled sound chip");
      if( sound_chip_->is_enabled()) {
        sound_chip_->push_byte(ora_);
        ira_ = 0x0;
      }
      sound_chip_->disable();
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
    default:
      spdlog::get("SystemVIA")->warn("Set ORB 0x{:0X}", out_value);
      break;
  }
}

/*
 * Various behaviors based on the state of DDRA
 *
 */
uint8_t SystemVia::read_port_a() const {
  if( ddra_ == 0x7f) {
    // KBD handling

  }
  return 0x00;
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

/*
 * The 6502 can set or clear selected bits in the interrupt enable register without affecting the other bits.
 * This is accomplished by writing to the IER.
 * If bit 7 of the byte written is a 0 then each 1 in bits 0–6 will
 * clear the corresponding bit in the IER. For each zero in bits 0–6, the corresponding bit will not be affected.
 * Selected bits can be SET in a similar manner. In this case, bit 7 of the written byte should be set to 1.
 * Each 1 in bits 0–6 will then SET the selected bit. A zero will cause the corresponding bit to remain unaffected.
 * The contents of the IER can be read by the 6502. Bit 7 is then always read as a logic 1.
 */
uint8_t SystemVia::ier() const {
  spdlog::get("SystemVIA")->info("Get IER (0x{:02X})", (ier_ | 0x80));
  return 0x80 | ier_;
}

void SystemVia::set_ier(uint8_t value) {
  if (value & 0x80) {
    ier_ = ier_ & (~value);
  } else {
    ier_ = ier_ | value;
  }
  spdlog::get("SystemVIA")->info("Set IER (0x{:02X}) | TIMER 1 {} | TIMER 2 {} | CB1 (EOC) {} | CB2 (LPSTB) {} | SHIFT_REG {} | CA1 (VSYNC) {} | CA2 (KEYB) {} |", value,
                                 (ier_ & 0x40) ? "X" : " ",
                                 (ier_ & 0x20) ? "X" : " ",
                                 (ier_ & 0x10) ? "X" : " ",
                                 (ier_ & 0x08) ? "X" : " ",
                                 (ier_ & 0x04) ? "X" : " ",
                                 (ier_ & 0x02) ? "X" : " ",
                                 (ier_ & 0x01) ? "X" : " "
                                 );
}

void SystemVia::set_pcr(uint8_t value) {
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
  spdlog::get("SystemVIA")->info("Set PCR (0x{:02x})", value);
}