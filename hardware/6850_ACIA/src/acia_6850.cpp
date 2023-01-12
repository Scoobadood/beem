//
// Created by Dave Durbin on 3/1/2023.
//

#include "acia_6850.h"

#include "spdlog/spdlog-inl.h"
#include "spdlog/sinks/basic_file_sink.h"

Acia::Acia()//
{}

void mmio_read(uint16_t addr, Bus &bus) {
  spdlog::info("ACIA: Read from 0x{:04x}", addr);
}

void mmio_write(uint16_t addr, Bus &bus) {
  spdlog::info("ACIA: Write ({:02x}) to 0x{:04x}", bus.get_data(), addr);
}

void Acia::tick(Bus &bus) {
  auto addr = bus.get_address();
  if (addr < 0xfe08 || addr > 0xfe0f) return;
  auto read = bus.tst_RW();
  if (read) {
    mmio_read(addr, bus);
  } else {
    mmio_write(addr, bus);
  }
}
