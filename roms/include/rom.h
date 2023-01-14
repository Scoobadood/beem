//
// Created by Dave Durbin on 13/1/2023.
//

#ifndef BEEB_ROM_H_
#define BEEB_ROM_H_

#include "bus.h"

#include <vector>

class Rom {
 public:
  explicit Rom(std::ifstream &f);

  void tick(Bus &bus);

  // Convenience methods; used for tooling, not emulation.
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

#endif // BEEB_ROM_H_
