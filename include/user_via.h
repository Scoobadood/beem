//
// Created by Dave Durbin on 3/1/2023.
//

#ifndef M6502_SRC_USER_VIA_H_
#define M6502_SRC_USER_VIA_H_

#include "spdlog/spdlog-inl.h"
#include <vector>

class UserVia {
 public:
  UserVia();


  /*
   * User VIA, Data Direction Register A ($FE43) (aka 'DDRA')
   */
  void set_ddra(uint8_t value);
  uint8_t ddra() const;

  void set_ddrb(uint8_t value);
  uint8_t ddrb() const;

  void set_pcr(uint8_t value);

  void set_ora(uint8_t value);
  void set_orb(uint8_t value);

  uint8_t ira() const;
  uint8_t irb() const;

  uint8_t ier() const;
  void set_ier(uint8_t value);

  uint8_t ifr() const {return ifr_;}

 private:
  // Printer Port
  void write_port_a();
  uint8_t read_port_a() const;

  // User Port
  void write_port_b();
  uint8_t read_port_b();

  /*
   * There are two data direction registers DDRA and DDRB which specify whether the peripheral pins are to
   * operate as inputs or outputs. Placing a ‘0’ in a bit of a DDR will cause the corresponding bit of that
   * port to be defined as an input. A ‘1’ will cause it to be defined as an output.
   */
  uint8_t ddra_;
  uint8_t ddrb_;

  uint8_t orb_;
  uint8_t ora_;

  bool input_latching_;
  uint8_t ira_;
  uint8_t irb_;

  uint8_t ier_;

  uint8_t ifr_;

  uint8_t pcr_;
};

#endif //M6502_SRC_USER_VIA_H_
