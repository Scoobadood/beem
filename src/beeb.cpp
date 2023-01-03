//
// Created by Dave Durbin on 2/1/2023.
//

#include "beeb.h"

#include  <fstream>
#include  <iostream>
#include <spdlog/spdlog-inl.h>

Beeb::Beeb() {
  using namespace std;

  // Load bin file
  ifstream f("data/os120", ios::binary);
  if (!f.is_open()) {
    auto msg = fmt::format("Couldn't load data/os120");
    spdlog::error(msg);
    throw runtime_error(msg);
  }

  auto rom = vector<uint8_t>((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
  f.close();
  memory_ = new Memory(65536);
  memory_->insert(0xc000, rom);

  keyboard_ = new Keyboard();
  auto sound_chip = new SoundChip();
  system_via_ = new SystemVia(keyboard_, sound_chip);
  memory_->set_system_via(system_via_);

  memory_->set_user_via(new UserVia());

  memory_->set_acia(new Acia());

  cpu_ = new Cpu(false);
  cpu_->stack_pointer_ = 0xff;
  cpu_->pc_ = memory_->at(0xfffc) + memory_->at(0xfffd) * 256;
  clock_ = 0;
}

void Beeb::tick() {
  cpu_->tick(memory_, clock_);
  system_via_->tick();
  if( system_via_->interrupt_raised()) {
    cpu_->service_interrupt(memory_, clock_);
  }
  clock_++;
}
