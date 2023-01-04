//
// Created by Dave Durbin on 30/12/2022.
//

#ifndef M6502_INCLUDE_MEMORY_H_
#define M6502_INCLUDE_MEMORY_H_

#include "system_via.h"
#include "user_via.h"
#include "acia_6850.h"
#include "m6502.h"

#include <vector>
#include <iterator>
#include <fstream>

class Memory {
 public:
  explicit Memory(uint32_t sz);

  explicit Memory(std::ifstream &f);

  void push_stack(M6502 &cpu, uint8_t arg);
  uint8_t pop_stack(M6502 &cpu) const;

  uint8_t at(uint16_t addr) const;

  void set(uint16_t addr, uint8_t arg);

  void set_system_via(SystemVia *system_via);
  void set_user_via(UserVia *user_via);
  void set_acia(Acia *acia);

  void insert(uint16_t offset, std::vector<uint8_t> &data);

 private:
  uint8_t handle_mmio_reads(uint16_t addr) const;
  void handle_mmio_writes(uint16_t addr, uint8_t arg);

  SystemVia *system_via_;
  UserVia *user_via_;
  Acia *acia_;
  std::vector<uint8_t> memory_;
  uint32_t size_;
};

#endif //M6502_INCLUDE_MEMORY_H_
