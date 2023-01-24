#include "rom.h"

#include <fstream>
#include <spdlog/spdlog-inl.h>

void check_data_size(uint16_t sz) {
  if ((uint32_t)(sz & 0xffff)  < 0x10000) return;

  auto msg = fmt::format("ROM: Can't allocate {} bytes as it exceeds 64K addressable space.", sz);
  spdlog::critical(msg);
  throw std::runtime_error(msg);
}

void check_address_range(uint16_t base_addr, uint16_t sz) {
  uint32_t max_addr = (((uint32_t) base_addr) & 0xffff ) + (((uint32_t) sz) & 0xffff);
  if (max_addr <= 0x10000) return;

  auto msg = fmt::format("ROM: Can't allocate {} bytes at_bus {:04x} as it exceeds 64K addressable space.", sz, base_addr);
  spdlog::critical(msg);
  throw std::runtime_error(msg);
}

Rom::Rom(uint16_t sz, uint16_t bus_addr) //
        : bus_address_{bus_addr} {
  check_data_size(sz);
  check_address_range(bus_addr, sz);
  memory_ = std::make_shared<std::vector<uint8_t>>(sz, 0);
}

Rom::Rom(const std::vector<uint8_t> &data, uint16_t bus_addr)//
        : bus_address_{bus_addr} {
  check_data_size(data.size());
  check_address_range(bus_addr, data.size());
  memory_ = std::make_shared<std::vector<uint8_t>>(data.size(), 0);
  memory_->insert(memory_->end(), data.begin(), data.end());
}

Rom::Rom(const std::string &file_name, uint16_t bus_addr) //
        : bus_address_{bus_addr} {
  std::ifstream file{file_name, std::ios::in | std::ios::binary};
  if (file.fail()) {
    auto msg = fmt::format("ROM: Failed to load ROM data from file {}", file_name);
    spdlog::critical(msg);
    throw std::runtime_error(msg);
  }
  memory_ = std::make_shared<std::vector<uint8_t>>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  check_data_size(memory_->size());
  check_address_range(bus_addr, memory_->size());
}

void Rom::tick(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if( addr < bus_address_ || addr >= (bus_address_ + memory_->size())) return;

  // Read and write memory
  if (bus->tst_RW()) {
    auto data = memory_->at(addr-bus_address_);
    bus->set_data(data);
  } else {
    auto data = bus->get_data();
    memory_->at(addr-bus_address_) = data;
  }
}

uint8_t Rom::at_bus(uint16_t addr) const {
  if (addr < bus_address_) {
    auto msg = fmt::format("ROM: Invalid address access {:04x} for ROM at_bus base address {:04x}", addr, bus_address_);
    spdlog::critical(msg);
    throw std::runtime_error(msg);
  }
  return memory_->at(addr - bus_address_);
}
