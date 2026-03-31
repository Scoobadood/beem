#ifndef BEEB_HARDWARE_DRAM_H_
#define BEEB_HARDWARE_DRAM_H_

#include "bus.h"
#include "clock.h"

#include <vector>
#include <spdlog/spdlog.h>

class DRAM {
public:
  /**
   * Construct DRAM of given size and bus address.
   * @param sz The number of bytes of memory.
   * @param bus_address The base address of the first byte of memory in this DRAM.
   * NB bus_address + sz cannot by 0x10000 or greater as this is out of the adressabel range of the BBC
   */
  DRAM(uint16_t sz, uint16_t bus_address, std::shared_ptr<Clock> clk);

  /**
   * Load the memory from binary data in the specified file.
   * @return true if it's successful or else logs an error message and returns false.
   */
  bool load(const std::string &file_name);

  /**
   * Load the given data into memory at the specified point.
   */
  void load(const std::vector<uint8_t>& data, uint16_t addr);

  /**
   * Perform reads and writes to bus
   *
   * @param bus
   */
  void tick(const std::shared_ptr<Bus> &bus);

  /**
   * Peek memory: Convenience method; used for tooling, not emulation.
   * addr is assumed to be a bus address.
   */
  [[nodiscard]] uint8_t at_bus(uint16_t addr) const;

  /**
   * Return const ref to underlying memory
   */
  inline std::shared_ptr<std::vector<uint8_t>> data() {
    return memory_;
  }

  /**
   * Return raw pointer to the first byte of DRAM (for O(1) page-table dispatch).
   */
  inline uint8_t* raw_ptr() {
    return memory_->data();
  }

private:
  /* Address on the bus */
  uint16_t bus_address_;

  /* The stored memory */
  std::shared_ptr<std::vector<uint8_t>> memory_;

  /* A copy f the system clock */
  std::shared_ptr<Clock> clock_;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<spdlog::logger> bus_dance_logger_;
};

#endif // BEEB_HARDWARE_DRAM_H_
