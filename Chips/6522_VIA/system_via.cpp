//
// Created by Dave Durbin on 30/12/2022.
//

#include "system_via.h"
#include "keyboard.h"
#include "sound_76489.h"
#include "spdlog/spdlog-inl.h"
#include "spdlog/sinks/basic_file_sink.h"

static const std::string control_modes[] = {
    "negative edges active on input",
    "independent interrupt; input negative edge",
    "positive edges active on input",
    "independent interrupt; input positive edge",
    "handshake output mode",
    "pulse output mode",
    "low output",
    "high output"
};


const uint8_t IFR_T1_TIMED_OUT = 0x01 << 6;

SystemVia::SystemVia(Keyboard *keyboard, SoundChip *sound_chip) {
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

  timer_1_latch_ = 0;
  timer_1_count_ = 0;
  timer_1_mode_ = 0;
  timer_1_running_ = false;

  pcr_ = 0;
  acr_ = 0;

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
  spdlog::get("SystemVIA")->info("Read IRB (0x{:02x})", irb_);
  return irb_;
}

uint8_t SystemVia::ira() const {
  spdlog::get("SystemVIA")->info("Read IRA (0x{:02x})", ira_);
  // TODO Input latching
  return ira_;
}

void SystemVia::write_port_a() {
  if (ddra_ == 0x7f) {
    // Keyboard probing
    auto key_code = ora_ & 0x7f;
    if (keyboard_->key_pressed(key_code)) {
      ira_ = 0x80 | key_code;
      spdlog::get("SystemVIA")->info("Probe key 0x{:02x} (pushed)", key_code);
    } else {
      ira_ = 0x00;
      spdlog::get("SystemVIA")->info("Probe key 0x{:02x} (not pushed)", key_code);
    }
    return;
  } else if (ddra_ == 0xff) {
    spdlog::get("SystemVIA")->info("Poking the sound chip");
  } else {
    spdlog::warn("Not Implemented: SystemVIA::write_port_a() with ddra==0x{:02x}", ddra_);
  }
}

void SystemVia::write_port_b() {
  auto out_value = (orb_ & 0xf) & ddrb_;
  switch (out_value) {
    case 0:spdlog::get("SystemVIA")->info("Enabled sound chip");
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
    case 8:spdlog::get("SystemVIA")->info("Disabled sound chip");
      if (sound_chip_->is_enabled()) {
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
    default:spdlog::get("SystemVIA")->warn("Set ORB 0x{:0X}", out_value);
      break;
  }
}

/*
 * Various behaviors based on the state of DDRA
 *
 */
uint8_t SystemVia::read_port_a() const {
  if (ddra_ == 0x7f) {
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
  spdlog::get("SystemVIA")->info("Get IER (0x{:02x})", (ier_ | 0x80));
  return 0x80 | ier_;
}

void SystemVia::set_ier(uint8_t value) {
  if (value & 0x80) {
    ier_ = ier_ | value;
  } else {
    ier_ = ier_ & (~value);
  }
  spdlog::get("SystemVIA")->info(
      "Set IER (0x{:02x}) | TIMER 1 {} | TIMER 2 {} | CB1 (EOC) {} | CB2 (LPSTB) {} | SHIFT_REG {} | CA1 (VSYNC) {} | CA2 (KEYB) {} |",
      value,
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

  uint8_t ca2_control_mode = (value & 0x0e) >> 1;
  uint8_t cb2_control_mode = (value & 0xd0) >> 5;
  spdlog::get("SystemVIA")->info(
      "Set PCR (0x{:02x})  CA1 (VSync): {}  CA2 (Keyb) Mode: {}  CB1 (EOC): {}  CB2 (LPSTB) Mode: {}",
      value,
      (value & 0x01) ? "positive active edge" : "negative active edge",
      control_modes[ca2_control_mode],
      (value & 0x10) ? "positive active edge" : "negative active edge",
      control_modes[cb2_control_mode]
  );
}

uint8_t SystemVia::pcr() const {
  uint8_t ca2_control_mode = (pcr_ & 0x0e) >> 1;
  uint8_t cb2_control_mode = (pcr_ & 0xd0) >> 5;
  spdlog::get("SystemVIA")->info(
      "Read PCR (0x{:02x})  CA1 (VSync): {}  CA2 (Keyb) Mode: {}  CB1 (EOC): {}  CB2 (LPSTB) Mode: {}",
      pcr_,
      (pcr_ & 0x01) ? "positive active edge" : "negative active edge",
      control_modes[ca2_control_mode],
      (pcr_ & 0x10) ? "positive active edge" : "negative active edge",
      control_modes[cb2_control_mode]
  );
  return pcr_;
}

/*
 * System VIA, Interrupt Flag Register ($FE4D) (aka 'IFR')
 * bit 0 = key pressed
 * bit 1 = vertical sync occurred
 * bit 2 = shift register timeout (unused)
 * bit 3 = lightpen strobe off screen
 * bit 4 = analogue conversion completed
 * bit 5 = timer 2 has timed out (used for speech)
 * bit 6 = timer 1 has timed out (100Hz signal)
 * bit 7 = (when reading) master interrupt flag (0-6 invalid if clear)
 *
 * Used in interrupt code:
 * Reading
 * -------
 * If bit 7 is set then the System VIA caused the current interrupt. The remaining bits can
 * then be checked to see the exact cause.
 *
 * Writing
 * -------
 * Clear bit 7 and set a bit 0-6 to clear that interrupt.
 */
uint8_t SystemVia::ifr() const {
  spdlog::get("SystemVIA")->info(
      "Read IFR (0x{:02x}) | MASTER {} | TIMER 1 {} | TIMER 2 {} | CB1 (EOC) {} | CB2 (LPSTB) {} | SHIFT_REG {} | CA1 (VSYNC) {} | CA2 (KEYB) {} |",
      ifr_,
      (ifr_ & 0x80) ? "X" : " ",
      (ifr_ & IFR_T1_TIMED_OUT) ? "X" : " ",
      (ifr_ & 0x20) ? "X" : " ",
      (ifr_ & 0x10) ? "X" : " ",
      (ifr_ & 0x08) ? "X" : " ",
      (ifr_ & 0x04) ? "X" : " ",
      (ifr_ & 0x02) ? "X" : " ",
      (ifr_ & 0x01) ? "X" : " "
  );

  return ifr_;
}

void SystemVia::set_ifr(uint8_t data) {
  if (data & 0x80) {
    return;
  }
  ifr_ = (ifr_ & ~data);
  if ((ifr_ & 0x7f) == 0) {
    ifr_ = 0;
  }
  spdlog::get("SystemVIA")->info(
      "Set IFR (0x{:02x}) | MASTER {} | TIMER 1 {} | TIMER 2 {} | CB1 (EOC) {} | CB2 (LPSTB) {} | SHIFT_REG {} | CA1 (VSYNC) {} | CA2 (KEYB) {} |",
      ifr_,
      (ifr_ & 0x80) ? "X" : " ",
      (ifr_ & IFR_T1_TIMED_OUT) ? "X" : " ",
      (ifr_ & 0x20) ? "X" : " ",
      (ifr_ & 0x10) ? "X" : " ",
      (ifr_ & 0x08) ? "X" : " ",
      (ifr_ & 0x04) ? "X" : " ",
      (ifr_ & 0x02) ? "X" : " ",
      (ifr_ & 0x01) ? "X" : " "
  );
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
void SystemVia::set_acr(uint8_t value) {
  uint32_t sr_mode = (value & 0x1c) >> 2;
  timer_1_mode_ = (value & IFR_T1_TIMED_OUT) >> 4;
  spdlog::get("SystemVIA")->info(
      "Set ACR (0x{:02x}) | Pulse PB7 {} | TIMER 1 {} | TIMER 2 {} | SR Mode {} | PB Latch enable {} | PA Latch enable {} |",
      value,
      (value & 0x80) ? "X" : " ",
      timer_1_mode_ ? "Free run" : "One shot",
      (value & 0x20) ? "Pulse count" : "One shot",
      sr_mode,
      (value & 0x02) ? "X" : " ",
      (value & 0x01) ? "X" : " "
  );
  acr_ = value;
}

void SystemVia::set_T1_latch_high(uint8_t data) {
  timer_1_latch_ = (data << 8) | (timer_1_latch_ & 0xff);
  spdlog::get("SystemVIA")->info("Set T1 latch high: 0x{:02x} (0x{:04x})", data, timer_1_latch_);
}

void SystemVia::set_T1_latch_low(uint8_t data) {
  timer_1_latch_ = (data) | (timer_1_latch_ & 0xff00);
  spdlog::get("SystemVIA")->info("Set T1 latch low: 0x{:02x} (0x{:04x})", data, timer_1_latch_);
}

void SystemVia::set_T1_counter_low(uint8_t data) {
  timer_1_latch_ = (data) | (timer_1_latch_ & 0xff00);
  spdlog::get("SystemVIA")->info("Set T1 counter low (latch only): 0x{:02x} (0x{:04x})", data, timer_1_latch_);
}

void SystemVia::set_T1_counter_high(uint8_t data) {
  // Place into latch
  timer_1_latch_ = (data << 8) | (timer_1_latch_ & 0xff);

  // Now transfer latches to counter
  timer_1_count_  = timer_1_latch_;
  ifr_ &= (~IFR_T1_TIMED_OUT);
  timer_1_running_ = true;

  spdlog::get("SystemVIA")->info("Set T1 counter high. Timer running: 0x{:02x} (0x{:04x})", data, timer_1_count_);
}
void SystemVia::tick() {
  if( timer_1_running_) {
    timer_1_count_--;
    if( timer_1_count_ == 0) {
      if( timer_1_mode_) {
        // Free running
        // Invert PB7: Shouldn't be permitted by DDRB
        if( ddrb_ & 0x80) {
          spdlog::warn("PB7 is configured as write. This is unexpected.");
          orb_ ^= 0x80;
          write_port_b();
        }
        // Raise the IRQ
        if( ier_ & IFR_T1_TIMED_OUT) {
          ifr_ |= (0x80 | IFR_T1_TIMED_OUT);
        }
        // Reset counter
        timer_1_count_ = timer_1_latch_;
      } else {
        timer_1_running_ = false;
        if( ier_ & IFR_T1_TIMED_OUT) {
          ifr_ |= (0x80 | IFR_T1_TIMED_OUT);
        }
        spdlog::warn( "Timer 1 timed out in one shot mode. Not yet fully supported, in particular PB7 pulsing.");
      }
    }
  }
}