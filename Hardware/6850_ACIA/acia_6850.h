//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef M6502_INCLUDE_ACIA_6850_H_
#define M6502_INCLUDE_ACIA_6850_H_

#include <stdint.h>

class Acia {
 public:
  Acia();

  /*
  ; ACIA 6850, Status Register ($FE08)
  ;
  ; This is used for reading values. By convention (for clarity) we use the Control Register
  ; for writing values. Even though they both refer to
  ; the same memory location, they have different meanings depending on whether it's being
  ; read or written.
  ;
  ;     bit 0 - set when a receiver interrupt is generated
  ;     bit 1 - set when a transmit interrupt is generated
  ;     bit 2 - set when a Data Carrier Detect ('DCD') interrupt is generated
  ;     bit 3 - set if the 6850 is not clear to send ('CTS')
  ;     bit 4 - framing error     } only valid if bit 0 set
  ;     bit 5 - receiver over run } only valid if bit 0 set
  ;     bit 6 - parity error      } only valid if bit 0 set
  ;     bit 7 - set if the 6850 generated the current interrupt
  ;
  */
  uint8_t status() const;

  /*
  ; ACIA 6850, Control Register ($FE08)
  ;
  ; This is used for writing values. By convention (for clarity) we use the Status Register
  ; (See .acia6850StatusRegister below) for reading values. Even though they both refer to
  ; the same memory location, they have different meanings depending on whether it's being
  ; read or written.
  ;
  ;     bits 0,1 - the counter divide select bits (CR0/CR1)
  ;          %00 - divide counter by 1
  ;          %01 - divide counter by 16                     (used for 1200 baud tape)
  ;          %10 - divide counter by 64 (default for RS-423) (used for 300 baud tape)
  ;          %11 - master reset
  ;     bit 2 - set means odd parity; otherwise even parity
  ;     bit 3 - set means 1 stop bit; otherwise 2 stop bits
  ;     bit 4 - set means 8 bit word; otherwise 7 bit word
  ;     bits 5,6:
  ;          %00 - 'Request To Send' ('RTS') low, transmit interrupt disabled
  ;          %01 - RTS low, transmit interrupt enabled
  ;          %10 - RTS high, transmit interrupt disabled
  ;          %11 - RTS low, break level on data output, transmit interrupt disabled
  ;     bit 7 - enable receive data register full, overrun, DCD transition interrupts
  ;
  ;         DCD = Data Carrier Detect interrupt occurs when the tone at the end of a cassette
  ;               block is discontinued
  ;
  ;     RTS low is the active state ('Request To Send')
  */
  void set_ctl(uint8_t data);

  /*
   * Data register. Write TDR, read RDR
   */
  uint8_t rdr() const;
  void set_tdr(uint8_t data);

  /*
   ;
   ; Serial ULA, Control Register ($FE10)
   ;
   ; See ula.png
   ;
   ;     bits 0-2 - transmit rate
   ;     bits 3-5 - receive rate
   ;                   %000    19200 baud
   ;                   %001     9600 baud
   ;                   %010     4800 baud
   ;                   %011     2400 baud
   ;                   %100     1200 baud
   ;                   %101      300 baud
   ;                   %110      150 baud
   ;                   %111       75 baud
   ;     bit 6 - if set, the RS-423 system has control of the serial system; otherwise the cassette system has control
   ;     bit 7 - if set, switch on the cassette motor and relay
   ;
   */
  void set_ula_ctl(uint8_t data);
 private:
  uint8_t status_;
  uint8_t control_;
  uint8_t data_;
  uint8_t serial_ula_;
  uint32_t tx_baud_;
  uint32_t rx_baud_;
};

#endif //M6502_INCLUDE_ACIA_6850_H_
