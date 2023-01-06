#include "m6502.h"
#include "memory.h"
#include "cycle_handler.h"
#include "opcodes.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>

#include <spdlog/spdlog-inl.h>

void debug_flags_regs(M6502 *cpu) {
  spdlog::info("        a:${:02x}  x:${:02x}  y:${:02x} {}{}{}{}{}{}{}{}",
               cpu->A(), cpu->X(), cpu->Y(),
               cpu->tstN() ? "N" : "n",
               cpu->tstV() ? "V" : "v",
               (cpu->flags() & FLAG_X) ? "X" : "x",
               (cpu->flags() & FLAG_B) ? "B" : "b",
               cpu->tstD() ? "D" : "d",
               (cpu->flags() & FLAG_I) ? "I" : "i",
               cpu->tstZ() ? "Z" : "z",
               cpu->tstC() ? "C" : "c"
  );
}

void debug_stack(M6502 *cpu, Memory *memory) {
  std::stringstream s;
  auto sp = cpu->SP();
  for (auto spc = 255; spc >= sp - 1; --spc) {
    s << ((spc == sp) ? " [" : "  ");
    s << "0x" << std::setw(2) << std::hex << std::setfill('0') << (int) memory->at(0x100 | spc);
    s << ((spc == sp) ? "] " : "  ");
  }

  spdlog::info("        sp:$1{:02x}:  {}", sp, s.str());
}

void debug(M6502 *cpu, Memory *memory) {
  auto opcode = memory->at(cpu->PC());
  auto op = OpCode::for_value(opcode);
  auto data_size = op.bytes - 1;
  uint16_t arg = 0;
  if (data_size == 1) {
    arg = memory->at(cpu->PC() + 1);
  }
  if (data_size == 2) {
    arg = memory->at(cpu->PC() + 1) + (memory->at(cpu->PC() + 2) * 256);
  }

  std::string msg;
  switch (op.addressing_mode) {
    case OpCode::Accumulator:msg = fmt::format("{:5s} A", op.name);
      break;

    case OpCode::Immediate:msg = fmt::format("{:5s} #${:02x}", op.name, arg);
      break;

    case OpCode::Absolute:msg = fmt::format("{:5s} ${:04x}", op.name, arg);
      break;

    case OpCode::AbsoluteIndexedX:msg = fmt::format("{:5s} ${:04x},X", op.name, arg);
      break;

    case OpCode::AbsoluteIndexedY:msg = fmt::format("{:5s} ${:04x},Y", op.name, arg);
      break;

    case OpCode::Indirect:msg = fmt::format("{:5s} (${:04x})", op.name, arg);
      break;

    case OpCode::ZeroPage:msg = fmt::format("{:5s} ${:02x}", op.name, arg);
      break;

    case OpCode::ZeroPageIndexedX:msg = fmt::format("{:5s} ${:02x},X", op.name, arg);
      break;

    case OpCode::ZeroPageIndexedY:msg = fmt::format("{:5s} ${:02x},Y", op.name, arg);
      break;

    case OpCode::IndirectIndexedX:msg = fmt::format("{:5s} (${:02x},X)", op.name, arg);
      break;

    case OpCode::IndirectIndexedY:msg = fmt::format("{:5s} (${:02x}),Y", op.name, arg);
      break;

    case OpCode::Implied:msg = fmt::format("{:5s}", op.name);
      break;

    case OpCode::Relative:msg = fmt::format("{:5s} ${:04x}", op.name, cpu->PC() + 2 + (int8_t) arg);
      break;
  }
  spdlog::info("0x{:04x}  {:15}", cpu->PC(), msg);
}

int main(int argc, const char *argv[]) {
  using namespace std;

  bool should_debug = ((argc > 1) && (strlen(argv[1]) == 2) && (argv[1][0] == '-') && (argv[1][1] == 'd'));

  // Load bin file
  ifstream f("data/6502_functional_test.bin", ios::binary);
  if (!f.is_open()) {
    cerr << "File read failed" << endl;
    return 0;
  }
  auto memory = Memory(f);
  f.close();

  M6502 cpu;

  // Histoyr buffer for PC
  uint32_t buffer_size = 200;
  uint32_t dup_pc_count = 0;
  uint32_t buffer_idx = 0;
  std::vector<uint16_t> pc_history(buffer_size, 0);

  // Pull reset low
  uint64_t pins = 0;

  auto start = std::chrono::high_resolution_clock::now();

  while (true) {
    pins = cpu.tick(pins);
    set_RST(pins);

    // Make sure code execution starts at 0x400, not reset vector
    auto addr = get_address(pins);

    // Special case reset
    if (addr == 0xfffc) {
      set_data(pins, 0);
      continue;
    } else if (addr == 0xfffd) {
      set_data(pins, 4);
      continue;
    }

    pins = memory.tick(pins);

    // Log PC to history buffer
    if (tst_SYNC(pins)) {
      if (should_debug) {
        debug_flags_regs(&cpu);
        debug_stack(&cpu, &memory);
        debug(&cpu, &memory);
      }

      auto pc = cpu.PC();
      if (pc == pc_history.at(buffer_idx)) {
        dup_pc_count++;
        if (dup_pc_count == 8) {
          if (pc == 0x3469) {
            auto stop = std::chrono::high_resolution_clock::now();
            // Get duration. Substart timepoints to
            // get duration. To cast it to proper unit
            // use duration cast method
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
            spdlog::info("All tests passed in {:02}:{:02}", (duration.count() / 60), duration.count() % 60);
            return EXIT_SUCCESS;
          } else {
            spdlog::critical("Stuck in a loop at PC: 0x{:04x}", pc);
            for (auto i = 1; i <= buffer_size; ++i) {
              spdlog::info("{:04x}", pc_history.at((buffer_idx + i) % buffer_size));
            }
            return EXIT_FAILURE;
          }
        }
      } else {
        dup_pc_count = 0;
        buffer_idx = (buffer_idx + 1) % buffer_size;
        pc_history.at(buffer_idx) = pc;
      }
    }
  }
}
