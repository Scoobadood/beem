/**
 * The bus has 16 address lines, 8 data lines and control lines.
 * All chips can talk to the bus.
 */

#ifndef BEEB_HARDWARE_MAIN_BUS_H_
#define BEEB_HARDWARE_MAIN_BUS_H_

#include <vector>

const uint32_t MAX_BUS_ADDR = 0x10000f;

const uint64_t PIN_DATA_MASK = 0x000000ff;
const uint64_t PIN_ADDR_MASK = 0x00ffff00;
const uint64_t PIN_SYNC = 0x01000000;
const uint64_t PIN_RST = 0x02000000;
const uint64_t PIN_RD_NOT_WR = 0x04000000;

class Bus {
 public:
  inline uint16_t get_address() const {
    return (pins_ & PIN_ADDR_MASK) >> 8;
  }

  void set_address(uint16_t address) {
    pins_ = (pins_ & ~PIN_ADDR_MASK) | ((address & 0xffff) << 8);
  }

  inline uint8_t get_data() const {
    return (pins_ & PIN_DATA_MASK);
  }

  inline void set_data(uint8_t data) {
    pins_ = (pins_ & ~PIN_DATA_MASK) | (data & 0xff);
  }

  inline bool tst_RST() const {
    return ( pins_ & PIN_RST);
  }

  inline void set_RST() {
    pins_ |= PIN_RST;
  }

  inline void clr_RST() {
    pins_ &= ~PIN_RST;
  }

  inline bool tst_RW() const {
    return (pins_ & PIN_RD_NOT_WR);
  }

  inline void set_RW() {
    pins_ |= PIN_RD_NOT_WR;
  }

  inline void clr_RW() {
    pins_ &= ~PIN_RD_NOT_WR;
  }

   inline bool tst_SYNC() const {
    return (pins_ & PIN_SYNC);
  }

  inline void set_SYNC() {
    pins_ |= PIN_SYNC;
  }

  inline void clr_SYNC() {
    pins_ &= ~PIN_SYNC;
  }

 private:
  uint64_t pins_;
};

#endif //BEEB_HARDWARE_MAIN_BUS_H_
