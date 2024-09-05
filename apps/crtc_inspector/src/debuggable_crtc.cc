#include "debuggable_crtc.h"

const uint16_t BASE_ADDRESS= 0xFE00;

DebuggableCrtc::DebuggableCrtc() : Crtc(BASE_ADDRESS) {
  bus_ = std::make_shared<Bus>();
}
void DebuggableCrtc::set_register(uint8_t reg, uint8_t value){
  bus_->set_address(BASE_ADDRESS);
  bus_->set_data(reg);
  bus_->clr_RW();
  tick(bus_);
  bus_->set_address(BASE_ADDRESS+1);
  bus_->set_data(value);
  bus_->clr_RW();
  tick(bus_);
}