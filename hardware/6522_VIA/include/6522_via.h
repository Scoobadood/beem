#ifndef BEEB_HARDWARE_6522_VIA_INCLUDE_H_
#define BEEB_HARDWARE_6522_VIA_INCLUDE_H_

#include "bus.h"
#include "data_connectors.h"

#include <cstdint>
#include <set>

class Via {
 public:
  explicit Via(uint16_t base_address);

  void tick(Bus &bus);

  void subscribe_port_a(const data_subscriber_8_bit_ptr &subscriber);
  void unsubscribe_port_a(const data_subscriber_8_bit_ptr &subscriber);
  void subscribe_port_b(const data_subscriber_8_bit_ptr &subscriber);
  void unsubscribe_port_b(const data_subscriber_8_bit_ptr &subscriber);
  void provide_port_a(data_provider_8_bit_ptr provider);

 private:
  void mmio_read(Bus &bus, uint8_t reg);
  void mmio_write(Bus &bus, uint8_t reg);
  void check_mmio(Bus &bus);
  void write_irq_enable(uint8_t data);

  void write_port_a(uint8_t data);
  void write_port_b(uint8_t data);
  uint8_t read_port_a();

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
  std::set<data_subscriber_8_bit_ptr> port_b_subscribers_;
  std::set<data_subscriber_8_bit_ptr> port_a_subscribers_;

  // Providers to ports
  std::set<data_provider_8_bit_ptr> port_a_providers_;
};

#endif //BEEB_HARDWARE_6522_VIA_INCLUDE_H_
