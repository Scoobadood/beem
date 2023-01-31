#ifndef BEEB_HARDWARE_6522_VIA_INCLUDE_H_
#define BEEB_HARDWARE_6522_VIA_INCLUDE_H_

#include "bus.h"
#include "data_connectors.h"

#include <cstdint>
#include <set>

class Via {
 public:
  explicit Via(uint16_t base_address);

  void tick(const std::shared_ptr<Bus>& bus);

  void subscribe_port_a(const data_subscriber_8_bit_ptr &subscriber);
  void unsubscribe_port_a(const data_subscriber_8_bit_ptr &subscriber);
  void subscribe_port_b(const data_subscriber_8_bit_ptr &subscriber);
  void unsubscribe_port_b(const data_subscriber_8_bit_ptr &subscriber);
  void provide_port_a(data_provider_8_bit_ptr provider);
  void provide_ca2(data_provider_8_bit_ptr provider);

  void set_ca1(uint8_t state);
  void set_cb1(uint8_t state);

 private:
  void raise_irq(uint8_t irq);
  void clear_irq(uint8_t irq);
  void mmio_read(const std::shared_ptr<Bus>& bus, uint8_t reg);
  void mmio_write(const std::shared_ptr<Bus>& bus, uint8_t reg);
  void check_irq();
  void check_timers();
  void check_ca2();
  void check_mmio(const std::shared_ptr<Bus>& bus);
  void write_irq_enable(uint8_t data);
  void write_pcr(uint8_t data);
  void write_acr(uint8_t data);
  void write_port_a(uint8_t data);
  void write_port_b(uint8_t data);
  uint8_t read_port_a();
  uint8_t read_port_b();

  uint16_t base_address_;

  uint8_t ddra_;
  uint8_t ira_;
  uint8_t ora_;
  uint8_t ca1_;
  uint8_t ca2_;
  uint8_t pa_latch_;
  uint8_t prev_ca1_;

  uint8_t ddrb_;
  uint8_t irb_;
  uint8_t orb_;
  uint8_t cb1_;
  uint8_t cb2_;
  uint8_t pb_latch_;
  uint8_t prev_cb1_;

  uint8_t ier_;
  uint8_t ifr_;
  uint8_t acr_;

  // PCR
  uint8_t pcr_;
  bool ca1_pos_active_edge_;
  bool cb1_pos_active_edge_;
  uint8_t ca2_ctl_;
  uint8_t cb2_ctl_;

  // Timers
  uint16_t timer1_count_;
  uint16_t timer1_latch_;
  uint8_t pb7_;
  uint16_t timer2_count_;
  uint8_t timer2_latch_;

  // Subscribers to Ports
  std::set<data_subscriber_8_bit_ptr> port_b_subscribers_;
  std::set<data_subscriber_8_bit_ptr> port_a_subscribers_;

  // Providers to ports
  std::set<data_provider_8_bit_ptr> port_a_providers_;
  std::set<data_provider_8_bit_ptr> port_b_providers_;
  std::set<data_provider_8_bit_ptr> ca2_providers_;
};

#endif //BEEB_HARDWARE_6522_VIA_INCLUDE_H_
