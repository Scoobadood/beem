//
// Created by Dave Durbin on 11/1/2023.
//

#ifndef BEEB_HW_COMMON_DATA_SUBSCRIBERS_H_
#define BEEB_HW_COMMON_DATA_SUBSCRIBERS_H_

#include <set>

class data_subscriber_8_bit {
public:
  explicit data_subscriber_8_bit(uint8_t mask) //
          : mask_{mask} //
          , data_{0} //
          , data_changed_{false} //
  {}

  [[nodiscard]] inline uint8_t mask() const { return mask_; }

  void set_data(uint8_t bit_num, bool value) {
    auto bit_mask = 0x01 << bit_num;
    if (mask_ & bit_mask) {
      if (value) {
        data_ |= bit_mask;
      } else {
        data_ &= ~bit_mask;
      }
      data_changed_ = true;
    }
  };

  [[nodiscard]] inline bool data_changed() const { return data_changed_; }

  inline uint8_t data() {
    data_changed_ = false;
    return data_;
  }

private:
  uint8_t mask_;
  uint8_t data_;
  bool data_changed_;
};

class data_provider_8_bit {
public:
  explicit data_provider_8_bit(uint8_t initial_data = 0) : data_{initial_data}, has_data_{false} {}

  [[nodiscard]] inline virtual bool has_data() const { return has_data_; }

  inline virtual uint8_t data() {
    has_data_ = false;
    return data_;
  }

  void provide_data(uint8_t data) {
    data_ = data;
    has_data_ = true;
  }

private:
  uint8_t data_;
  bool has_data_;
};

using data_subscriber_8_bit_ptr = std::shared_ptr<data_subscriber_8_bit>;
using data_provider_8_bit_ptr = std::shared_ptr<data_provider_8_bit>;

void notify_subscribers(const std::set<data_subscriber_8_bit_ptr> &subs,
                        uint8_t value,
                        uint8_t bits_changed);

#endif //BEEB_HW_COMMON_DATA_SUBSCRIBERS_H_
