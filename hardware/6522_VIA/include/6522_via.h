#ifndef BEEB_HARDWARE_6522_VIA_INCLUDE_H_
#define BEEB_HARDWARE_6522_VIA_INCLUDE_H_

#include "bus.h"
#include "data_subscribers.h"

#include <cstdint>
#include <set>

class Via {
 public:
  explicit Via(uint16_t base_address);

  void tick(Bus &bus);

  void subscribe_port_a(const data_subscriber_8_bit & subscriber);
  void unsubscribe_port_a(const data_subscriber_8_bit & subscriber);
  void subscribe_port_b(const data_subscriber_8_bit & subscriber);
  void unsubscribe_port_b(const data_subscriber_8_bit & subscriber);

 private:
  void mmio_read(Bus & bus, uint8_t reg);
  void mmio_write(Bus & bus, uint8_t reg);
  void check_mmio(Bus & bus);

  void write_port_b(uint8_t data);

  uint16_t base_address_;

  uint8_t ddra_;
  uint8_t ira_;
  uint8_t ora_;
  uint8_t ca1_;
  uint8_t ca2_;

  uint8_t ddrb_;
  uint8_t irb_;
  uint8_t orb_;
  uint8_t cb1_;
  uint8_t cb2_;

  uint8_t ier_;
  uint8_t ifr_;
  uint8_t acr_;
  uint8_t pcr_;

  // Subscribers to Ports
  std::set<data_subscriber_8_bit > port_b_subscribers_;
  std::set<data_subscriber_8_bit > port_a_subscribers_;
};

#endif //BEEB_HARDWARE_6522_VIA_INCLUDE_H_
