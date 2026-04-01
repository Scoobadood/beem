#include "adc.h"

#include <spdlog/sinks/basic_file_sink.h>

const uint8_t DL_WO = 0x00;
const uint8_t SR_RO = 0x00;
const uint8_t HI_RO = 0x01;
const uint8_t LO_RO = 0x02;

Adc::Adc(uint16_t base_address) //
        : base_address_{base_address} //
{
  adc_name_ = fmt::format("ADC@{:04x}", base_address);
  try {
    auto logger = spdlog::basic_logger_mt(adc_name_, "logs/" + adc_name_ + ".txt", true);
    logger->flush_on(spdlog::level::err);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
  logger_ = spdlog::get(adc_name_);
}

void Adc::tick(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < base_address_) return;
  if (addr > base_address_ + LO_RO) return;

  // Read
  if (bus->tst_RW()) {
    uint8_t data = bus->get_data();
    switch (addr - base_address_) {
      case SR_RO:
        data = (0x80) | (mode_ << 2) | channel_;
        logger_->info("Read {:02x} from SR", base_address_, data);
        break;

      case HI_RO:
        data = 0xaa;
        logger_->info("Read {:02x} from HI_RO", base_address_, data);
        break;

      case LO_RO:
        data = 0xf0;
        logger_->info("Read {:02x} from LO_RO", base_address_, data);
        break;

      default:
        logger_->info("Tried to read unknown reg {:04x}", base_address_, addr);
        break;
    }
  } else {
    if (addr - base_address_ != DL_WO) return;
    auto data = bus->get_data();
    mode_ = (data >> 2) & 0x1;
    channel_ = (data & 0x3);
    logger_->info("Value {:02x} written", base_address_, data);
    logger_->info("        : Input channel {}, {} bit precision, starting.", channel_, (mode_) ? "10" : "8");
  }
}