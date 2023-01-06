//
// Created by Dave Durbin on 30/12/2022.
//

#ifndef M6502_INCLUDE_MEMORY_H_
#define M6502_INCLUDE_MEMORY_H_

#include <vector>

class Memory {
 public:
  explicit Memory(uint32_t sz);

  explicit Memory(std::ifstream &f);

  uint64_t tick(uint64_t pins);

  // Convenience methods; used for tooling, not emulation.
  /**
   * Bulkload ROM data or test code.
   */
  void insert(uint16_t offset, std::vector<uint8_t> &data);

  /**
   * Peek memory
   */
   uint8_t at(uint16_t addr) const;


 private:
  std::vector<uint8_t> memory_;
};

#endif //M6502_INCLUDE_MEMORY_H_
