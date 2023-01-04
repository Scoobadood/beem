//
// Created by Dave Durbin on 3/1/2023.
//

#include "cycle_handler.h"

extern const std::map<uint16_t, CycleHandler> cycle_handlers;

CycleHandler cycle_handler(uint16_t ir) {
  auto iter = cycle_handlers.find(ir);
  if (iter == cycle_handlers.end()) {
    return nullptr;
  }
  return iter->second;
}

/**
 * Perform the core of comparison functions for CPY, CPX nd CMP,
 * setting flags appropriately.
 * @param cpu - for settign flags
 * @param reg The register
 * @param val Thre value to compare to reg.
 */
void do_cmp(M6502 *cpu, uint8_t reg, uint8_t val) {
  uint16_t t = reg - val;
  cpu->setNZ(t & 0xff);
  if (t & 0xff00) cpu->setC(); else cpu->clrC();
}

/**
 * Perform addition
 * @param cpu For flags
 * @param val To add to acc
 */
void do_add(M6502 *cpu, uint8_t val) {
  if (cpu->tstD()) {
    /* Decimal mode (credit goes to MAME) */
    uint8_t carry = (cpu->tstC()) ? 1 : 0;
    cpu->clrC();
    cpu->clrN();
    cpu->clrZ();
    cpu->clrV();

    // Low nybble addition
    uint8_t al = (cpu->a() & 0x0f) + (val & 0x0f) + carry;
    if (al > 9) {
      al += 6;
    }

    // High nybble
    uint8_t ah = (cpu->a() >> 4) + (val >> 4) + (al > 0x0f);

    if ((uint8_t) (cpu->a() + val + carry) == 0) {
      cpu->setZ();
    } else if (ah & 0x08) {
      cpu->setN();
    }
    if (~(cpu->a() ^ val) & (cpu->a() ^ (ah << 4)) & 0x80) {
      cpu->setV();
    }
    if (ah > 9) {
      ah += 6;
    }
    if (ah > 15) {
      cpu->setC();
    }
    cpu->set_a((ah << 4) | (al & 0x0f));
  } else {
    /* Default (binary) addition */
    uint16_t sum = cpu->a() + val + (cpu->tstC() ? 1 : 0);

    // Clear carry and overflow then check if we should set them.
    cpu->clrC();
    cpu->clrV();
    cpu->setNZ(sum);

    // Set V if needed
    if (~(cpu->a() ^ val) & (cpu->a() ^ sum) & 0x80) {
      cpu->setV();
    }

    // Set C if needed
    if (sum & 0xff00) {
      cpu->setC();
    }
    cpu->set_a(sum & 0xff);
  }
}

void fetch(M6502 *cpu, uint64_t &pins) {
  set_address(pins, cpu->pc());
  set_SYNC(pins);
}

const std::map<uint16_t, CycleHandler> cycle_handlers = {


    // BPL #
    {0x100, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0x101, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
      cpu->set_temp_addr(cpu->pc() + (int8_t) get_data(pins));
      // Check if we should take the branch
      if (cpu->tstN()) fetch(cpu, pins);
    }},
    {0x102, [](M6502 *cpu, uint64_t &pins) {
      // Put un-page wrapped address on the bus
      set_address(pins, (cpu->pc() & 0xff00) | (cpu->temp_address() & 0xff));
      if ((cpu->temp_address() & 0xff00) == (cpu->pc() & 0xff00)) {
        cpu->set_pc(cpu->temp_address());
//        cpu->irq_pip >>= 1;
//        cpu->nmi_pip >>= 1;
        fetch(cpu, pins);
      }
    }},
    {0x103, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_pc(cpu->temp_address());
      fetch(cpu, pins);
    }},

    // CLC implied
    {0x180, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0x181, [](M6502 *cpu, uint64_t &pins) {
      cpu->clrC();
      fetch(cpu, pins);
    }},

    // EOR #
    {0x490, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0x491, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_a(cpu->a() ^ get_data(pins));
      cpu->setNZ(cpu->a());
      fetch(cpu, pins);
    }},

    // JMP
    {0x4c0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0x4c1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0x4c2, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_pc(get_data(pins) << 8 | cpu->temp_address());
      fetch(cpu, pins);
    }},


    // ADC #
    {0x690, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0x691, [](M6502 *cpu, uint64_t &pins) {
      do_add(cpu, get_data(pins));
      fetch(cpu, pins);
    }},


    // DEY
    {0x880, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0x881, [](M6502 *cpu, uint64_t &pins) {
      cpu->dec_y();
      cpu->setNZ(cpu->y());
      fetch(cpu, pins);
    }},

    // STA Absolute
    {0x8d0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0x8d1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0x8d2, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, get_data(pins) << 8 | cpu->temp_address());
      set_data(pins, cpu->a());
      clr_RW(pins);
    }},
    {0x8d3, [](M6502 *cpu, uint64_t &pins) {
      fetch(cpu, pins);
    }},

    // TYA Implied
    {0x980, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0x981, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_a(cpu->y());
      cpu->setNZ(cpu->a());
      fetch(cpu, pins);
    }},

    // TXS Implied
    {0x9a0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0x9a1, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_sp(cpu->x());
      fetch(cpu, pins);
    }},

    // LDY #
    {0xa00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xa01, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_y(get_data(pins));
      cpu->setNZ(cpu->y());
      fetch(cpu, pins);
    }},


    // LDX #
    {0xa20, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xa21, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_x(get_data(pins));
      cpu->setNZ(cpu->x());
      fetch(cpu, pins);
    }},

    // LDA #
    {0xa90, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xa91, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_a(get_data(pins));
      cpu->setNZ(cpu->a());
      fetch(cpu, pins);
    }},

    // TAX Implied
    {0xaa0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0xaa1, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_x(cpu->a());
      cpu->setNZ(cpu->x());
      fetch(cpu, pins);
    }},

    // LDA Abs
    {0xad0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xad1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0xad2, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, get_data(pins) << 8 | cpu->temp_address());
    }},
    {0xad3, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_a(get_data(pins));
      cpu->setNZ(cpu->a());
      fetch(cpu, pins);
    }},

    // CPY #
    {0xc00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xc01, [](M6502 *cpu, uint64_t &pins) {
      do_cmp(cpu, cpu->y(), get_data(pins));
      fetch(cpu, pins);
    }},

    // CMP #
    {0xc90, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xc91, [](M6502 *cpu, uint64_t &pins) {
      do_cmp(cpu, cpu->a(), get_data(pins));
      fetch(cpu, pins);
    }},

    // DEX
    {0xca0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0xca1, [](M6502 *cpu, uint64_t &pins) {
      cpu->dec_x();
      cpu->setNZ(cpu->x());
      fetch(cpu, pins);
    }},

    // BNE #
    {0xd00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xd01, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
      cpu->set_temp_addr(cpu->pc() + (int8_t) get_data(pins));
      // Check if we should take the branch
      if (cpu->tstZ()) fetch(cpu, pins);
    }},
    {0xd02, [](M6502 *cpu, uint64_t &pins) {
      // Put un-page wrapped address on the bus
      set_address(pins, (cpu->pc() & 0xff00) | (cpu->temp_address() & 0xff));
      if ((cpu->temp_address() & 0xff00) == (cpu->pc() & 0xff00)) {
        cpu->set_pc(cpu->temp_address());
//        cpu->irq_pip >>= 1;
//        cpu->nmi_pip >>= 1;
        fetch(cpu, pins);
      }
    }},
    {0xd03, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_pc(cpu->temp_address());
      fetch(cpu, pins);
    }},

    // CLD implied
    {0xd80, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0xd81, [](M6502 *cpu, uint64_t &pins) {
      cpu->clrD();
      fetch(cpu, pins
      );
    }},

    // NOP
    {0xea0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
    }},
    {0xea1, [](M6502 *cpu, uint64_t &pins) {
      fetch(cpu, pins);
    }},

    // BEQ #
    {0xf00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->inc_pc());
    }},
    {0xf01, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->pc());
      cpu->set_temp_addr(cpu->pc() + (int8_t) get_data(pins));
      // Check if we should take the branch
      if (!cpu->tstZ()) fetch(cpu, pins);
    }},
    {0xf02, [](M6502 *cpu, uint64_t &pins) {
      // Put un-page wrapped address on the bus
      set_address(pins, (cpu->pc() & 0xff00) | (cpu->temp_address() & 0xff));
      if ((cpu->temp_address() & 0xff00) == (cpu->pc() & 0xff00)) {
        cpu->set_pc(cpu->temp_address());
//        cpu->irq_pip >>= 1;
//        cpu->nmi_pip >>= 1;
        fetch(cpu, pins);
      }
    }},
    {0xf03, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_pc(cpu->temp_address());
      fetch(cpu, pins);
    }},
};

