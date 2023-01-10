//
// Created by Dave Durbin on 3/1/2023.
//

#include "acia_6850.h"

#include "spdlog/spdlog-inl.h"
#include "spdlog/sinks/basic_file_sink.h"

Acia::Acia() {
  status_ = 0x00;

  try {
    auto logger = spdlog::basic_logger_mt("Acia", "logs/acia.txt", true);
    spdlog::flush_every((std::chrono::seconds) 5);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
}

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
uint8_t Acia::status() const {
  spdlog::get("Acia")->info("Read status ( IRQ: {}  PE: {}  RVR: {}  FE: {}  CTS: {}  DCD: {}  TDRE: {}  RDRE: {})",
                            (status_ & 0x80) ? "X" : " ",
                            (status_ & 0x40) ? "X" : " ",
                            (status_ & 0x20) ? "X" : " ",
                            (status_ & 0x10) ? "X" : " ",
                            (status_ & 0x08) ? "X" : " ",
                            (status_ & 0x04) ? "X" : " ",
                            (status_ & 0x02) ? "X" : " ",
                            (status_ & 0x01) ? "X" : " ");
  return status_;
}

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
void Acia::set_ctl(uint8_t data) {
  auto cd = (data & 0x03);
  auto mode = (data & 0x60) >> 4;
  spdlog::get("Acia")->info("Set CTL 0x{:02x}   {}  {}  {} {} {} {}", data,
                            (status_ & 0x80) ? "RXI enabled " : "",
                            (mode == 0)
                            ? "RTS low, TXI disabled"
                            : (mode == 1)
                              ? "RTS low, TXI enabled"
                              : (mode == 2)
                                ? "RTS high, TXI disabled"
                                : "RTS low, Brk lvl on data output, TXI disabled",

                            (status_ & 0x10) ? " 8 bit word " : " 7 bit word ",
                            (status_ & 0x08) ? " 1 stop bit " : " 2 stop bits ",
                            (status_ & 0x04) ? " odd parity " : " even parity ",
                            (cd == 3)
                            ? "Master reset"
                            : (cd == 2)
                              ? "Ctr Div 64 (300 baud tape and RS-423)"
                              : (cd == 1)
                                ? "Ctr Div 16 (1200 baud tape)"
                                : "Ctr Div 1");
  control_ = data;
}

/*
 * Data register. Write TDR, read RDR
 */
uint8_t Acia::rdr() const {
  spdlog::get("Acia")->info("Read RDR(0x{:02x})", data_);
  return data_;
}

void Acia::set_tdr(uint8_t data) {
  spdlog::get("Acia")->info("Wrote TDR(0x{:02x})", data);
  data_ = data;
}

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
void Acia::set_ula_ctl(uint8_t data) {
  const uint32_t baud[] = {19200, 9600, 4800, 2400, 1200, 300, 150, 75};
  auto tb = data & 0x07;
  auto rb = (data & 0x38) >> 3;
  tx_baud_ = baud[tb];
  rx_baud_ = baud[rb];
  spdlog::get("Acia")->info("Set ULA CTL 0x{:02x} TX: {}  RX: {}  CTL: {}  Tape: {}", data,
                            tx_baud_, rx_baud_,
                            (data & 0x40) ? "RS-423" : "Cassette",
                            (data & 0x80) ? "On" : "Off");
}
