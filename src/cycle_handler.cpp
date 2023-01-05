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
 * @param cpu - for setting flags
 * @param reg The register
 * @param val The value to compare to reg.
 */
void do_cmp(M6502 *cpu, uint8_t reg, uint8_t val) {
  uint16_t t = reg - val;
  cpu->setNZ(t & 0xff);

  // The carry flag is set when the value in memory is less than or equal to the accumulator,
  // reset when it is greater than the accumulator.
  if (t & 0xff00) cpu->clrC();
  else cpu->setC();
}

/**
 * Perform BCD addition
 */
void do_add_bcd(M6502 *cpu, uint8_t val) {
  /* Decimal mode (credit goes to MAME) */
  uint8_t carry = (cpu->tstC()) ? 1 : 0;
  cpu->clrC();
  cpu->clrN();
  cpu->clrZ();
  cpu->clrV();

  // Low nybble addition
  uint8_t al = (cpu->A() & 0x0f) + (val & 0x0f) + carry;
  if (al > 9) {
    al += 6;
  }

  // High nybble
  uint8_t ah = (cpu->A() >> 4) + (val >> 4) + (al > 0x0f);

  if ((uint8_t) (cpu->A() + val + carry) == 0) {
    cpu->setZ();
  } else if (ah & 0x08) {
    cpu->setN();
  }
  if (~(cpu->A() ^ val) & (cpu->A() ^ (ah << 4)) & 0x80) {
    cpu->setV();
  }
  if (ah > 9) {
    ah += 6;
  }
  if (ah > 15) {
    cpu->setC();
  }
  cpu->setA((ah << 4) | (al & 0x0f));
}

/**
 * Perform binary addition
 */
void do_add_binary(M6502 *cpu, uint8_t val) {
  // Add with carry flag
  uint16_t sum = cpu->A() + val + (cpu->tstC() ? 1 : 0);

  // Clear carry and overflow then check if we should set them.
  cpu->clrC();
  cpu->clrV();

  // Set V if needed
  if (~(cpu->A() ^ val) & (cpu->A() ^ sum) & 0x80) {
    cpu->setV();
  }

  // Set C if needed
  if (sum & 0xff00) {
    cpu->setC();
  }
  cpu->setA(sum & 0xff);
  cpu->setNZ(cpu->A());
}

/**
 * Perform addition
 * @param cpu For flags
 * @param val To add to acc
 */
void do_add(M6502 *cpu, uint8_t val) {
  if (cpu->tstD()) {
    do_add_bcd(cpu, val);
  } else {
    do_add_binary(cpu, val);
  }
}

void fetch(M6502 *cpu, uint64_t &pins) {
  set_address(pins, cpu->PC());
  set_SYNC(pins);
}

/*
 * Common branch steps
 */
void do_branch_0(M6502 *cpu, uint64_t &pins) { set_address(pins, cpu->incPC()); }
void do_branch_1(M6502 *cpu, uint64_t &pins, uint8_t flag, bool branch_if_set) {
  set_address(pins, cpu->PC());
  cpu->set_temp_addr(cpu->PC() + (int8_t) get_data(pins));

  // Can we avoid the branch?
  bool flag_set = (cpu->flags() & flag);
  if (flag_set == branch_if_set) return;

  // Skip branch
  fetch(cpu, pins);
}
void do_branch_2(M6502 *cpu, uint64_t &pins) {
  // Put un-page wrapped address on the bus
  set_address(pins, (cpu->PC() & 0xff00) | (cpu->temp_address() & 0xff));
  if ((cpu->temp_address() & 0xff00) == (cpu->PC() & 0xff00)) {
    cpu->setPC(cpu->temp_address());
//        cpu->irq_pip >>= 1;
//        cpu->nmi_pip >>= 1;
    fetch(cpu, pins);
  }
}
void do_branch_3(M6502 *cpu, uint64_t &pins) {
  cpu->setPC(cpu->temp_address());
  fetch(cpu, pins);
}

const std::map<uint16_t, CycleHandler> cycle_handlers = {


    // BPL #
    {0x100, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0x101, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_N, false);
    }},
    {0x102, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0x103, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},

    // CLC implied
    {0x180, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x181, [](M6502 *cpu, uint64_t &pins) {
      cpu->clrC();
      fetch(cpu, pins);
    }},

    // PLP implied
    {0x280, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x281, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->SP());
    }},
    {0x282, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incSP());
    }},
    {0x283, [](M6502 *cpu, uint64_t &pins) {
      cpu->set_flags((get_data(pins) | FLAG_B) & ~FLAG_X);
      fetch(cpu, pins);
    }},



    // BMI #
    {0x300, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0x301, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_N, true);
    }},
    {0x302, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0x303, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},


    // PHA
    {0x480, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x481, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, 0x100 | cpu->decSP());
      set_data(pins, cpu->A());
      clr_RW(pins);
    }},
    {0x482, [](M6502 *cpu, uint64_t &pins) {
      fetch(cpu, pins);
    }},

    // EOR #
    {0x490, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0x491, [](M6502 *cpu, uint64_t &pins) {
      cpu->setA(cpu->A() ^ get_data(pins));
      cpu->setNZ(cpu->A());
      fetch(cpu, pins);
    }},

    // JMP
    {0x4c0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0x4c1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0x4c2, [](M6502 *cpu, uint64_t &pins) {
      cpu->setPC(get_data(pins) << 8 | cpu->temp_address());
      fetch(cpu, pins);
    }},


    // BVC #
    {0x500, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0x501, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_V, false);
    }},
    {0x502, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0x503, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},


    // PLA implied
    {0x680, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x681, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, 0x100 | cpu->SP());
    }},
    {0x682, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, 0x100 | cpu->incSP());
    }},
    {0x683, [](M6502 *cpu, uint64_t &pins) {
      cpu->setA(get_data(pins));
      cpu->setNZ(cpu->A());
      fetch(cpu, pins);
    }},



    // ADC #
    {0x690, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0x691, [](M6502 *cpu, uint64_t &pins) {
      do_add(cpu, get_data(pins));
      fetch(cpu, pins);
    }},


    // BVS #
    {0x700, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0x701, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_V, true);
    }},
    {0x702, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0x703, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},



    // DEY
    {0x880, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x881, [](M6502 *cpu, uint64_t &pins) {
      cpu->decY();
      cpu->setNZ(cpu->Y());
      fetch(cpu, pins);
    }},

    // TXA Implied
    {0x8a0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x8a1, [](M6502 *cpu, uint64_t &pins) {
      cpu->setA(cpu->X());
      cpu->setNZ(cpu->A());
      fetch(cpu, pins);
    }},


    // STA Absolute
    {0x8d0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0x8d1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0x8d2, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, get_data(pins) << 8 | cpu->temp_address());
      set_data(pins, cpu->A());
      clr_RW(pins);
    }},
    {0x8d3, [](M6502 *cpu, uint64_t &pins) {
      fetch(cpu, pins);
    }},

    // BCC #
    {0x900, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0x901, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_C, false);
    }},
    {0x902, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0x903, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},


    // TYA Implied
    {0x980, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x981, [](M6502 *cpu, uint64_t &pins) {
      cpu->setA(cpu->Y());
      cpu->setNZ(cpu->A());
      fetch(cpu, pins);
    }},


    // TXS Implied
    {0x9a0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0x9a1, [](M6502 *cpu, uint64_t &pins) {
      cpu->setSP(cpu->X());
      fetch(cpu, pins);
    }},


    // LDY #
    {0xa00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xa01, [](M6502 *cpu, uint64_t &pins) {
      cpu->setY(get_data(pins));
      cpu->setNZ(cpu->Y());
      fetch(cpu, pins);
    }},


    // LDX #
    {0xa20, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xa21, [](M6502 *cpu, uint64_t &pins) {
      cpu->setX(get_data(pins));
      cpu->setNZ(cpu->X());
      fetch(cpu, pins);
    }},


    // TAY Implied
    {0xa80, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0xa81, [](M6502 *cpu, uint64_t &pins) {
      cpu->setY(cpu->A());
      cpu->setNZ(cpu->Y());
      fetch(cpu, pins);
    }},


    // LDA #
    {0xa90, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xa91, [](M6502 *cpu, uint64_t &pins) {
      cpu->setA(get_data(pins));
      cpu->setNZ(cpu->A());
      fetch(cpu, pins);
    }},


    // TAX Implied
    {0xaa0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0xaa1, [](M6502 *cpu, uint64_t &pins) {
      cpu->setX(cpu->A());
      cpu->setNZ(cpu->X());
      fetch(cpu, pins);
    }},


    // LDA Abs
    {0xad0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xad1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0xad2, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, get_data(pins) << 8 | cpu->temp_address());
    }},
    {0xad3, [](M6502 *cpu, uint64_t &pins) {
      cpu->setA(get_data(pins));
      cpu->setNZ(cpu->A());
      fetch(cpu, pins);
    }},

    // BCS #
    {0xb00, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0xb01, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_C, true);
    }},
    {0xb02, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0xb03, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},


    // TSX Implied
    {0xba0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0xba1, [](M6502 *cpu, uint64_t &pins) {
      cpu->setX(cpu->SP());
      cpu->setNZ(cpu->X());
      fetch(cpu, pins);
    }},


    // CPY #
    {0xc00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xc01, [](M6502 *cpu, uint64_t &pins) {
      do_cmp(cpu, cpu->Y(), get_data(pins));
      fetch(cpu, pins);
    }},


    // CMP #
    {0xc90, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xc91, [](M6502 *cpu, uint64_t &pins) {
      do_cmp(cpu, cpu->A(), get_data(pins));
      fetch(cpu, pins);
    }},


    // DEX
    {0xca0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0xca1, [](M6502 *cpu, uint64_t &pins) {
      cpu->decX();
      cpu->setNZ(cpu->X());
      fetch(cpu, pins);
    }},


    // CMP abs
    {0xcd0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xcd1, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
      cpu->set_temp_addr_low(get_data(pins));
    }},
    {0xcd2, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, get_data(pins) << 8 | cpu->temp_address());
    }},
    {0xcd3, [](M6502 *cpu, uint64_t &pins) {
      do_cmp(cpu, cpu->A(), get_data(pins));
      fetch(cpu, pins);
    }},


    // BNE #
    {0xd00, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0xd01, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_Z, false);
    }},
    {0xd02, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0xd03, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},


    // CLD implied
    {0xd80, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0xd81, [](M6502 *cpu, uint64_t &pins) {
      cpu->clrD();
      fetch(cpu, pins
      );
    }},


    // CPX #
    {0xe00, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->incPC());
    }},
    {0xe01, [](M6502 *cpu, uint64_t &pins) {
      do_cmp(cpu, cpu->X(), get_data(pins));
      fetch(cpu, pins);
    }},


    // NOP
    {0xea0, [](M6502 *cpu, uint64_t &pins) {
      set_address(pins, cpu->PC());
    }},
    {0xea1, [](M6502 *cpu, uint64_t &pins) {
      fetch(cpu, pins);
    }},


    // BEQ #
    {0xf00, [](M6502 *cpu, uint64_t &pins) {
      do_branch_0(cpu, pins);
    }},
    {0xf01, [](M6502 *cpu, uint64_t &pins) {
      do_branch_1(cpu, pins, FLAG_Z, true);
    }},
    {0xf02, [](M6502 *cpu, uint64_t &pins) {
      do_branch_2(cpu, pins);
    }},
    {0xf03, [](M6502 *cpu, uint64_t &pins) {
      do_branch_3(cpu, pins);
    }},
};

const std::map<uint8_t, OpCode> op_codes = {
    {(uint8_t) 0x69, {2, 2, false, "adc", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0x65, {2, 3, false, "adc", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x75, {2, 4, false, "adc", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x6d, {3, 4, false, "adc", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x7d, {3, 4, true, "adc", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x79, {3, 4, true, "adc", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0x61, {2, 6, false, "adc", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0x71, {2, 5, false, "adc", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0x29, {2, 2, false, "and", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0x25, {2, 3, false, "and", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x35, {2, 4, false, "and", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x2d, {3, 4, false, "and", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x3d, {3, 4, true, "and", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x39, {3, 4, true, "and", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0x21, {2, 6, false, "and", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0x31, {2, 5, false, "and", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0x0a, {1, 2, false, "asl", OpCode::AddressingMode::Accumulator}},
    {(uint8_t) 0x06, {2, 5, false, "asl", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x16, {2, 6, false, "asl", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x0e, {3, 6, false, "asl", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x1e, {3, 7, false, "asl", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x90, {2, 2, true, "bcc", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0xb0, {2, 2, true, "bcs", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0xf0, {2, 2, true, "beq", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0x30, {2, 2, true, "bmi", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0xd0, {2, 2, true, "bne", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0x10, {2, 2, true, "bpl", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0x50, {2, 2, true, "bvc", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0x70, {2, 2, true, "bvs", OpCode::AddressingMode::Relative}},
    {(uint8_t) 0x00, {1, 7, false, "brk", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x18, {1, 2, false, "clc", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xd8, {1, 2, false, "cld", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x58, {1, 2, false, "cli", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xb8, {1, 2, false, "clv", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x24, {2, 3, false, "bit", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x2c, {3, 4, false, "bit", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xc9, {2, 2, false, "cmp", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xc5, {2, 3, false, "cmp", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xd5, {2, 4, false, "cmp", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0xcd, {3, 4, false, "cmp", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xdd, {3, 4, true, "cmp", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0xd9, {3, 4, true, "cmp", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0xc1, {2, 6, false, "cmp", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0xd1, {2, 5, false, "cmp", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0xe0, {2, 2, false, "cpx", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xe4, {2, 3, false, "cpx", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xec, {3, 4, false, "cpx", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xc0, {2, 2, false, "cpy", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xc4, {2, 3, false, "cpy", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xcc, {3, 4, false, "cpy", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xc6, {2, 5, false, "dec", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xd6, {2, 6, false, "dec", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0xce, {3, 6, false, "dec", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xde, {3, 7, false, "dec", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0xca, {1, 2, false, "dex", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x88, {1, 2, false, "dey", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x49, {2, 2, false, "eor", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0x45, {2, 3, false, "eor", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x55, {2, 4, false, "eor", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x4d, {3, 4, false, "eor", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x5d, {3, 4, true, "eor", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x59, {3, 4, true, "eor", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0x41, {2, 6, false, "eor", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0x51, {2, 5, true, "eor", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0xe6, {2, 5, false, "inc", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xf6, {2, 6, false, "inc", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0xee, {3, 6, false, "inc", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xfe, {3, 7, false, "inc", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0xe8, {1, 2, false, "inx", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xc8, {1, 2, false, "iny", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x4c, {3, 3, false, "jmp", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x6c, {3, 5, false, "jmp", OpCode::AddressingMode::Indirect}},
    {(uint8_t) 0x20, {3, 6, false, "jsr", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xa5, {2, 2, false, "lda", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xa9, {2, 3, false, "lda", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xad, {3, 4, false, "lda", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xb5, {3, 4, false, "lda", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0xb9, {3, 4, true, "lda", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0xbd, {3, 4, true, "lda", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0xa1, {2, 6, false, "lda", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0xb1, {2, 5, true, "lda", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0xa2, {2, 2, false, "ldx", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xa6, {2, 3, false, "ldx", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xb6, {2, 4, false, "ldx", OpCode::AddressingMode::ZeroPageIndexedY}},
    {(uint8_t) 0xae, {3, 4, false, "ldx", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xbe, {3, 4, true, "ldx", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0xa0, {2, 2, false, "ldy", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xa4, {2, 3, false, "ldy", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xb4, {2, 4, false, "ldy", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0xac, {3, 4, false, "ldy", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xbc, {3, 4, true, "ldy", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x4a, {1, 2, false, "lsr", OpCode::AddressingMode::Accumulator}},
    {(uint8_t) 0x46, {2, 5, false, "lsr", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x56, {2, 6, false, "lsr", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x4e, {3, 6, false, "lsr", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x5e, {3, 7, false, "lsr", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0xea, {1, 2, false, "nop", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x09, {2, 2, false, "ora", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0x05, {2, 3, false, "ora", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x15, {2, 4, false, "ora", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x0d, {3, 4, false, "ora", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x1d, {3, 4, true, "ora", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x19, {3, 4, true, "ora", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0x01, {2, 6, false, "ora", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0x11, {2, 5, true, "ora", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0x48, {1, 3, false, "pha", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x08, {1, 3, false, "php", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x68, {1, 4, false, "pla", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x28, {1, 4, false, "plp", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x2a, {1, 2, false, "rol", OpCode::AddressingMode::Accumulator}},
    {(uint8_t) 0x26, {2, 5, false, "rol", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x36, {2, 6, false, "rol", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x2e, {3, 6, false, "rol", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x3e, {3, 7, false, "rol", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x6a, {1, 2, false, "ror", OpCode::AddressingMode::Accumulator}},
    {(uint8_t) 0x66, {2, 5, false, "ror", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x76, {2, 6, false, "ror", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x6e, {3, 6, false, "ror", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x7e, {3, 7, false, "ror", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x40, {1, 6, false, "rti", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x60, {1, 6, false, "rts", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xe9, {2, 2, false, "sbc", OpCode::AddressingMode::Immediate}},
    {(uint8_t) 0xe5, {2, 3, false, "sbc", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0xf5, {2, 4, false, "sbc", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0xed, {3, 4, false, "sbc", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xfd, {3, 4, true, "sbc", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0xf9, {3, 4, true, "sbc", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0xe1, {2, 6, false, "sbc", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0xf1, {2, 5, true, "sbc", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0x38, {1, 2, false, "sec", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xf8, {1, 2, false, "sed", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x78, {1, 2, false, "sei", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x85, {2, 3, false, "sta", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x95, {2, 4, false, "sta", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x8d, {3, 4, false, "sta", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x9d, {3, 5, false, "sta", OpCode::AddressingMode::AbsoluteIndexedX}},
    {(uint8_t) 0x99, {3, 5, false, "sta", OpCode::AddressingMode::AbsoluteIndexedY}},
    {(uint8_t) 0x81, {2, 6, false, "sta", OpCode::AddressingMode::IndirectIndexedX}},
    {(uint8_t) 0x91, {2, 6, false, "sta", OpCode::AddressingMode::IndirectIndexedY}},
    {(uint8_t) 0x86, {2, 3, false, "stx", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x96, {2, 4, false, "stx", OpCode::AddressingMode::ZeroPageIndexedY}},
    {(uint8_t) 0x8e, {3, 4, false, "stx", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0x84, {2, 3, false, "sty", OpCode::AddressingMode::ZeroPage}},
    {(uint8_t) 0x94, {2, 4, false, "sty", OpCode::AddressingMode::ZeroPageIndexedX}},
    {(uint8_t) 0x8c, {3, 4, false, "sty", OpCode::AddressingMode::Absolute}},
    {(uint8_t) 0xaa, {1, 2, false, "tax", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xa8, {1, 2, false, "tay", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0xba, {1, 2, false, "tsx", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x8a, {1, 2, false, "txa", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x9a, {1, 2, false, "txs", OpCode::AddressingMode::Implied}},
    {(uint8_t) 0x98, {1, 2, false, "tya", OpCode::AddressingMode::Implied}},
};