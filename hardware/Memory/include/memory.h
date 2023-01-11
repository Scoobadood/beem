#ifndef M6502_INCLUDE_MEMORY_H_
#define M6502_INCLUDE_MEMORY_H_

#include "bus.h"

#include <vector>

class Memory {
 public:
  explicit Memory(uint32_t sz);

  explicit Memory(std::ifstream &f);

  void tick(Bus & bus);

  // Convenience methods; used for tooling, not emulation.
  /**
   * Bulkload ROM data or test code.
   */
  void insert(uint16_t offset, std::vector<uint8_t> &data);

  /**
   * Peek memory
   */
   [[nodiscard]] uint8_t at(uint16_t addr) const;

  /**
   * Return const ref to underlying memory
   */
  [[nodiscard]] inline std::vector<uint8_t> data() const {
    return memory_;
  }

 private:
  std::vector<uint8_t> memory_;
};

#endif //M6502_INCLUDE_MEMORY_H_
