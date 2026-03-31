#include "dram.h"
#include "bus.h"
#include "clock.h"

#include <spdlog/spdlog-inl.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iterator>
#include <fstream>
#include <utility>

DRAM::DRAM(uint16_t sz, uint16_t bus_addr, std::shared_ptr<Clock> clk)
        : bus_address_{bus_addr} //
        , clock_{std::move(clk)} //
{
  if (bus_addr + sz >= MAX_BUS_ADDR) {
    auto msg = fmt::format("DRAM: Can't allocate {} bytes of DRAM at_bus 0x{:04} as it exceeds 32K addressable space.",
                           sz, bus_addr);
    spdlog::critical(msg);
  }

  memory_ = std::make_shared<std::vector<uint8_t>>(sz);

  try {
    auto logger = spdlog::basic_logger_mt("DRAM", "logs/dram-log.txt", true);
    logger->flush_on(spdlog::level::err);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::warn("Log init failed for DRAM: {}", ex.what());
  }
  logger_ = spdlog::get("DRAM");
  bus_dance_logger_ = spdlog::get("BusDance");
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
  // Ignore stuff not for DRAM
  auto addr = bus->get_address();
  if (addr < bus_address_ || addr >= (bus_address_ + memory_->size())) {
    return;
  }


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
  logger_->debug("at_bus( 0x{:04x} )", addr);

  return memory_->at(addr - bus_address_);
}

/**
 * Load the given data into memory at the specified point.
 */
void DRAM::load(const std::vector<uint8_t>& data, uint16_t addr){
  for( auto b : data) {
    memory_->at(addr - bus_address_) = b;
    addr++;
  }
}
