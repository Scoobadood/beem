/*
 * 6522 (Versatile Interface Adapter, VIA)
 *
 * A VIA has two sets of 8 I/O lines with two associated control lines, known as Ports A and B.
 * Each I/O line can be set to input or output individually using the VIA's Data Direction registers;
 * the control lines (CA1/CA2, CB1/CB2) act as handshake signals for their respective ports.
 * The input and output registers are latched. The VIA also contains two 16-bit programmable timer/counters and a shift
 * register.
 *
 * The BBC Micro contains two VIAs. One (IC3) is dedicated to internal system operation, the other (IC69) is available
 * for system expansion.
 *
 * Port A of the system VIA acts as a slow data bus which connects to the keyboard, the sound generator (IC18) and
 * speech system chips (IC98, IC99). Port B drives an addressable latch which is used to provide read and write strobe
 * signals for the speech interface, the keyboard and the sound generator chip. Also, coming from this latch (IC32) are
 * control lines C0 and C1 which indicate the amount of RAM devoted to the display memory to be 16K, 8K, 10K or 20K.
 * Outputs 6 and 7 of the addressable latch drive the caps lock and shift lock LEDs on the keyboard. Two I/O lines on
 * Port B are used to input the two 'fire button' signals from the paddle connector SK6 and two more lines are used as
 * response lines from the speech interface. Each time the system VIA is written to, the latch connected to port B is
 * strobed by a flipflop (half of IC31) which is triggered from the 1MHz clock signal. Port A control line 1 (CA1) is
 * used for detecting keyboard activity, CA2 is connected to the video vertical sync, to generate the Start of Vertical
 * Sync event. CB1 takes the ADC End Of Conversion signal, and CB2 is used for the Light Pen Strobe signal.
 */
#ifndef M6502_SRC_VIA_H_
#define M6502_SRC_VIA_H_

#include "spdlog/spdlog-inl.h"
#include "keyboard.h"
#include "sound_76489.h"

/*

  ; ***************************************************************************************
  ;
  ; System VIA, Timer 1 registers ($FE44-7)
  ;
  ; This is a 1Mhz countdown timer. An IRQ is triggered when the timer reaches zero. The OS
  ; uses this timer as a 100Hz timer to update various parts of the OS. It is expected to
  ; remain as a 100Hz timer if the OS is to continue working properly. User VIA Timers are
  ; available for user programs instead.
  ; See .irq1CheckSystemVIA100HzTimer.
  ;
  ; Timer 1 can be configured in one of two modes by writing to the ACR
  ; (see .systemVIAAuxiliaryControlRegister):
  ;
  ; One-shot mode:
  ;   .systemVIATimer1LatchLow and .systemVIATimer1CounterHigh form a 16 bit countdown value.
  ;   Write to .systemVIATimer1LatchLow first then writing to .systemVIATimer1CounterHigh
  ;   starts the timer. When the timer is complete a timer IRQ interrupt is generated. This
  ;   only happens once.
  ;
  ; Free-run mode (aka 'Continuous interrupts'):
  ;   .systemVIATimer1LatchLow and .systemVIATimer1LatchHigh are initialised to the initial
  ;   timeout value for the timer. The timer starts when .systemVIATimer1CounterHigh is
  ;   also written. Unlike one-shot mode, once the timeout interrupt has happened the counter
  ;   is reset to the values in the latches and the process repeats. The process can be stopped
  ;   by writing .systemVIATimer1CounterHigh, by reading .systemVIATimer1CounterLow, or by
  ;   writing to the interrupt flag.
  ;   This is the mode set by the OS for Timer 1 at startup. See .setUpPage2.
  ;
  ; ***************************************************************************************
  .systemVIATimer1CounterLow                  = $FE44     ;
  .systemVIATimer1CounterHigh                 = $FE45     ;
  .systemVIATimer1LatchLow                    = $FE46     ;
  .systemVIATimer1LatchHigh                   = $FE47     ;

  ; ***************************************************************************************
  ;
  ; System VIA, Timer 2 registers ($FE48-9)
  ;
  ; Timer 2 (like Timer 1) is a 1MHz countdown timer with an IRQ being generated when the
  ; counter reaches zero. It is used by the OS to update the Speech system if present. It also
  ; has two modes of operation, selected by writing to the ACR. (See .systemVIAAuxiliaryControlRegister).
  ;
  ; One-shot mode:
  ;   This is similar to Timer 1 (above). Write the low byte of the timer first
  ;   .systemVIATimer2CounterLow then writing to the high byte of the counter
  ;   .systemVIATimer2CounterHigh starts the timer. When the timer is countdown reaches zero
  ;   a timer IRQ interrupt is generated. This only happens once.
  ;
  ; Pulse counting mode:
  ;   This is unlike Timer 1. It counts down the number of negative going pulses applied to
  ;   System VIA input pin PB6. Firstly write to .systemVIATimer2CounterLow, then writing to
  ;   .systemVIATimer2CounterHigh will start the countdown. When PB6 is pulsed low for the
  ;   appropriate number of times then an IRQ interrupt occurs. This only happens once. This
  ;   is the default mode as initialised at startup.
  ;
  ; Timer 2 is started by the Speech system (if present) as needed to time Speech. The timer
  ; is cleared by the OS when a Timer 2 IRQ is received (see .irq1CheckSystemVIASpeech).
  ; It is also cleared at startup. See .setUpPage2.
  ; ***************************************************************************************
  .systemVIATimer2CounterLow                  = $FE48     ;
  .systemVIATimer2CounterHigh                 = $FE49     ;

  ; ***************************************************************************************
  ;
  ; The System VIA, Shift Register ($FE4A)
  ;
  ; This is not used in this OS. It is designed to be used for serial data I/O by shifting
  ; bits one at a time under the control of an internal modulo-8 counter.
  ; See NAUG Section 22.4.9, Page 395.
  ;
  ; ***************************************************************************************
  .systemVIAShiftRegister                     = $FE4A     ;
 */

#include <vector>

class SystemVia {
 public:
  SystemVia(Keyboard *keyboard, SoundChip *sound_chip);

  bool interrupt_raised() const { return ifr_ & 0x80;}

  /*
   * System VIA, Data Direction Register A ($FE43) (aka 'DDRA')
   *
   * The keyboard, sound and speech systems use Data Direction Register A. Each bit of DDRA
   * indicates whether data can be written or read on that bit when data is accessed via
   * .systemVIARegisterANoHandshake.
   *
   * This is similar to DDRB. Unlike DDRB, the OS modifies DDRA frequently to set the appropriate bits for accessing
   * the device (often in the IRQ interrupt code). Once set, data is read or written to .systemVIARegisterANoHandshake
   * as needed.
   *
   * Sound:    When outputting sound, DDRA is set to %11111111 meaning all bits of data
   *           that are subsequently written to .systemVIARegisterANoHandshake are output bits.
   *           (See .sendToSoundChipFlagsAreadyPushed)
   *
   * Speech:   For speech, DDRA is set to %00000000 (for reading) or %11111111 (for writing) as
   *           needed. (See .readWriteSpeechProcessorPushedFlags)
   *
   * Keyboard: When reading the keyboard, DDRA is set to (%011111111). The key to read is written
   *           into bits 0-6 of .systemVIARegisterANoHandshake, and the 'pressed' state of that
   *           key is then read from bit 7.
   *           (See .interrogateKeyboard)
   *           (See .scanKeyboard)
   */
  void set_ddra(uint8_t value);
  uint8_t ddra() const;

  void set_ddrb(uint8_t value);
  uint8_t ddrb() const;

  void set_ora(uint8_t value);
  void set_orb(uint8_t value);

  uint8_t ira() const;
  uint8_t irb() const;

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
  void set_pcr(uint8_t value);
  uint8_t pcr() const;

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
  void set_acr(uint8_t value);

  /*
  ;
  ; System VIA, Timer 1 registers ($FE44-7)
  ;
  ; This is a 1Mhz countdown timer. An IRQ is triggered when the timer reaches zero. The OS
  ; uses this timer as a 100Hz timer to update various parts of the OS. It is expected to
  ; remain as a 100Hz timer if the OS is to continue working properly. User VIA Timers are
  ; available for user programs instead.
  ; See .irq1CheckSystemVIA100HzTimer.
  ;
  ; Timer 1 can be configured in one of two modes by writing to the ACR
  ; (see .systemVIAAuxiliaryControlRegister):
  ;
  ; One-shot mode:
  ;   .systemVIATimer1LatchLow and .systemVIATimer1CounterHigh form a 16 bit countdown value.
  ;   Write to .systemVIATimer1LatchLow first then writing to .systemVIATimer1CounterHigh
  ;   starts the timer. When the timer is complete a timer IRQ interrupt is generated. This
  ;   only happens once.
  ;
  ; Free-run mode (aka 'Continuous interrupts'):
  ;   .systemVIATimer1LatchLow and .systemVIATimer1LatchHigh are initialised to the initial
  ;   timeout value for the timer. The timer starts when .systemVIATimer1CounterHigh is
  ;   also written. Unlike one-shot mode, once the timeout interrupt has happened the counter
  ;   is reset to the values in the latches and the process repeats. The process can be stopped
  ;   by writing .systemVIATimer1CounterHigh, by reading .systemVIATimer1CounterLow, or by
  ;   writing to the interrupt flag.
  ;   This is the mode set by the OS for Timer 1 at startup. See .setUpPage2.
  ;
  */
  void set_T1_counter_low(uint8_t data);
  void set_T1_counter_high(uint8_t data);
  void set_T1_latch_low(uint8_t data);
  void set_T1_latch_high(uint8_t data);

  bool is_sound_chip_enabled() const {
    return sound_chip_->is_enabled();
  }

  bool is_read_speech_enabled() const {
    return read_speech_enabled_;
  }

  bool is_write_speech_enabled() const {
    return write_speech_enabled_;
  }

  bool is_keyb_autoscan_enabled() const {
    return keyb_autoscan_enabled_;
  }

  bool caps_lock_led() const {
    return caps_lock_led_;
  }

  bool shift_lock_led() const {
    return shift_lock_led_;
  }

  /*
  The values of C0 and C1 together determine the start scroll address for the screen:

         C0   C1      Screen       Used in
                      Address   Regular MODEs
         ------------------------------------
          0    0      $4000           3
          0    1      $5800          4,5
          1    0      $6000           6
          1    1      $3000         0,1,2
   */
  uint16_t start_scroll_address() const {
    switch (c0_ * 2 + c1_) {
      case 0: return 0x4000;
      case 1: return 0x5800;
      case 2: return 0x6000;
      case 3: return 0x3000;
      default:spdlog::error("Invalid C0 ({}), C1 ({}) combination in SystemVIA", c0_, c1_);
        return 0x4000;
    }
  }

  /*
   * The 6502 can set or clear selected bits in the interrupt enable register without affecting the other bits.
   * This is accomplished by writing to the IER.
   * If bit 7 of the byte written is a 0 then each 1 in bits 0–6 will
   * clear the corresponding bit in the IER. For each zero in bits 0–6, the corresponding bit will not be affected.
   * Selected bits can be SET in a similar manner. In this case, bit 7 of the written byte should be set to 1.
   * Each 1 in bits 0–6 will then SET the selected bit. A zero will cause the corresponding bit to remain unaffected.
   * The contents of the IER can be read by the 6502. Bit 7 is then always read as a logic 1.
   *
   * bit 0 = key pressed
   * bit 1 = vertical sync occurred
   * bit 2 = shift register timeout (unused)
   * bit 3 = light pen strobe off screen
   * bit 4 = analogue conversion completed
   * bit 5 = timer 2 timed out (used for speech)
   * bit 6 = timer 1 timed out (100Hz signal)
   * bit 7 = enable/disable interrupt value (see below)
   *
   * Writing:
   * --------
   * To enable  an interrupt, write a byte with bit 7 set   and set the desired bit(s) (0-6).
   * To disable an interrupt, write a byte with bit 7 clear and set the desired bit(s) (0-6).
   *
   * Reading:
   * --------
   * Bits 0-6 are read as expected.
   * Bit 7 is always set when read.
   */
  uint8_t ier() const;
  void set_ier(uint8_t value);

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
  uint8_t ifr() const;
  void set_ifr(uint8_t);

  void tick();

 private:
  /*
   * Value   Effect
   * -------------------------
   * 0       Enable sound chip
   * 1       Enable Read Speech
   * 2       Enable Write Speech
   * 3       Disable Keyboard auto scanning
   * 4       Hardware scrolling - set C0=0 (See below)
   * 5       Hardware scrolling - set C1=0 (See below)
   * 6       Turn on CAPS LOCK LED
   * 7       Turn on SHIFT LOCK LED
   * 8       Disable sound chip
   * 9       Disable Read Speech
   * 10      Disable Write Speech
   * 11      Enable Keyboard auto scanning
   * 12      Hardware scrolling - set C0=1 (See below)
   * 13      Hardware scrolling - set C1=1 (See below)
   * 14      Turn off CAPS LOCK LED
   * 15      Turn off SHIFT LOCK LED
   */
  void write_port_b();

  /*
   * The keyboard, sound and speech systems use Data Direction Register A.
   * Each bit of DDRA indicates whether data can be written or read on that bit when data is accessed via
   * .systemVIARegisterANoHandshake. This is similar to DDRB. Unlike DDRB, the OS modifies
   * DDRA frequently to set the appropriate bits for accessing the device (often in the IRQ
   * interrupt code).
   *
   * Once set, data is read or written to .systemVIARegisterANoHandshake as needed. See .systemVIARegisterANoHandshake.
   * Sound:    When outputting sound, DDRA is set to %11111111 meaning all bits of data
   *           that are subsequently written to .systemVIARegisterANoHandshake are output bits.
   *           (See .sendToSoundChipFlagsAreadyPushed)
   *
   * Speech:   For speech, DDRA is set to %00000000 (for reading) or %11111111 (for writing) as
   *           needed. (See .readWriteSpeechProcessorPushedFlags)
   *
   * Keyboard: When reading the keyboard, DDRA is set to (%011111111). The key to read is written
   *           into bits 0-6 of .systemVIARegisterANoHandshake, and the 'pressed' state of that
   *           key is then read from bit 7.
   *           (See .interrogateKeyboard)
   *           (See .scanKeyboard)
   */
  void write_port_a();

  /*
   * Port A of the system VIA acts as a slow data bus which connects to the keyboard,
   * the sound generator (IC18) and speech system chips (IC98, IC99)
   */
  uint8_t read_port_a() const;

  /**
   * When reading from this get_address the top four bits are read:
   * bit 7:    Speech processor 'ready' signal
   * bit 6:    Speech processor 'interrupt' signal
   * bit 4-5:  joystick buttons (bit is zero when button pressed)
   * @return
   */
  uint8_t read_port_b();

    /*
     * There are two data direction registers DDRA and DDRB which specify whether the peripheral pins are to
     * operate as inputs or outputs. Placing a ‘0’ in a bit of a DDR will cause the corresponding bit of that
     * port to be defined as an input. A ‘1’ will cause it to be defined as an output.
     */
  uint8_t ddra_;
  /*
   * System VIA, Data Direction Register B ($FE42) (aka 'DDRB')
   *
   * When writing data into Register B (.systemVIARegisterB), the bits that are set on DDRB
   * indicate which bits are actually written into Register B. The bits that are clear on DDRB
   * are used to read from Register B.
   *
   * DDRB is only written once on startup where it is initialised to %00001111
   * (see .setUpSystemVIA) and the OS expects it to remain that way. Only the bottom four bits
   * of .systemVIARegisterB are used when writing, and only the upper four bits are read from
   * .systemVIARegisterB. See .systemVIARegisterB.
   */
  uint8_t ddrb_;
  uint8_t orb_;
  uint8_t ora_;
  bool input_latching_;
  uint8_t ira_;
  uint8_t irb_;
  bool read_speech_enabled_;
  bool write_speech_enabled_;
  bool keyb_autoscan_enabled_;
  bool caps_lock_led_;
  bool shift_lock_led_;
  uint8_t c0_;
  uint8_t c1_;

  uint8_t ier_;

  uint8_t ifr_;

  uint8_t pcr_;

  // Auxiliary Control Register
  uint8_t acr_;

  // Timer 1
  uint16_t timer_1_count_;
  uint16_t timer_1_latch_;
  uint8_t timer_1_mode_; // 0= one shot, 1 = free run
  bool timer_1_running_ = false;

  Keyboard *keyboard_;
  SoundChip *sound_chip_;
};

#endif //M6502_SRC_VIA_H_
