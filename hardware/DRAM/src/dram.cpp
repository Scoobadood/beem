#include "dram.h"
#include "bus.h"

#include <spdlog//spdlog-inl.h>

#include <iterator>
#include <fstream>

DRAM::DRAM(uint32_t sz, uint16_t bus_addr)
        : bus_address_{bus_addr} {
  if (bus_addr + sz >= 0x10000) {
    auto msg = fmt::format("DRAM: Can't allocate {} bytes of DRAM at_bus 0x{:04} as it exceeds 32K addressable space.",
                           sz, bus_addr);
    spdlog::critical(msg);
    throw std::runtime_error(msg);
  }
  memory_ = std::make_shared<std::vector<uint8_t>>(sz);
}

bool DRAM::load(const std::string &file_name) {
  std::ifstream file{file_name, std::ios::in | std::ios::binary};
  if (file.fail()) {
    spdlog::error("DRAM: Failed to load data from file {}", file_name);
    return false;
  }
  auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  auto max_data_size = 0x10000 - (((uint32_t) bus_address_) & 0xffff);
  if (data.size() > max_data_size) {
    spdlog::error("DRAM: Too much data in file {}. Expected max {:04x}, found {:04x}", file_name, max_data_size,
                  data.size());
    return false;
  }
  memory_->clear();
  memory_->insert(memory_->end(), data.begin(), data.end());
  return true;
}


void DRAM::tick(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < bus_address_ || addr >= (bus_address_ + memory_->size())) return;

  // Read and write memory
  if (bus->tst_RW()) {
    auto data = memory_->at(addr - bus_address_);
    bus->set_data(data);
  } else {
    auto data = bus->get_data();
    memory_->at(addr - bus_address_) = data;
  }
}

/**
 * Return the contents of the given address.
 * @param addr The address
 * @return The contents
 */
uint8_t DRAM::at_bus(uint16_t addr) const {
  return memory_->at(addr - bus_address_);
}

