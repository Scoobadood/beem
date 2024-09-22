#ifndef LIBS_BEEB_APPS_ACIA_TESTER_INCLUDE_FAKE_SULA_H_
#define LIBS_BEEB_APPS_ACIA_TESTER_INCLUDE_FAKE_SULA_H_

#include "2c198_sula.h"

class FakeSula : public AbstractSula {
 public:
  ~FakeSula() override = default;
  virtual void subscribe_to_carrier_detect(data_subscriber_8_bit_ptr subscriber) override {
    // NOOP
  };
  virtual void subscribe_to_cts(data_subscriber_8_bit_ptr subscriber) override {
    sub_ = subscriber;
  };
  void set_cts() {
    if (sub_) {
      sub_->set_data(0, true);
    }
  }
  void clear_cts() {
    if (sub_) {
      sub_->set_data(0, false);
    }
  }
  data_subscriber_8_bit_ptr sub_;
};

#endif //LIBS_BEEB_APPS_ACIA_TESTER_INCLUDE_FAKE_SULA_H_
