#ifndef LIBS_BEEB_APPS_ACIA_TESTER_INCLUDE_DEBUGGABLE_ACIA_H_
#define LIBS_BEEB_APPS_ACIA_TESTER_INCLUDE_DEBUGGABLE_ACIA_H_

#include "acia_6850.h"

class DebuggableAcia: public Acia {
 public:
  DebuggableAcia(uint16_t base_addr ) : Acia(base_addr){}

  inline uint8_t status_register() { return status_register_;}

  inline uint8_t tdr(){ return tdr_;}
  inline uint8_t rdr(){ return rdr_;}
  inline uint8_t tx_shift_register() { return tx_shift_register_;}
  inline uint8_t rx_shift_register() { return rx_shift_register_;}
  inline uint16_t base_address() { return base_addr_;}
  inline uint8_t control_register() { return control_register_;}
  inline bool cts() { return cts_;}
};

#endif //LIBS_BEEB_APPS_ACIA_TESTER_INCLUDE_DEBUGGABLE_ACIA_H_
