#include "cycle_handler.h"

// ir_ is built as (8-bit opcode << 4) | 4-bit cycle counter.
// Maximum value: 0xFF << 4 | 0xF = 0xFFF, so 0x1000 entries cover all cases.
static CycleHandler handler_table[0x1000] = {};

CycleHandler cycle_handler(uint16_t ir) {
  return handler_table[ir];
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

void do_sub_bcd(M6502 *cpu, uint8_t val) {
  uint8_t carry = cpu->tstC() ? 0 : 1;

  cpu->clrN();
  cpu->clrV();
  cpu->clrZ();
  cpu->clrC();

  uint16_t diff = cpu->A() - val - carry;
  uint8_t al = (cpu->A() & 0x0F) - (val & 0x0F) - carry;
  if ((int8_t) al < 0) {
    al -= 6;
  }
  uint8_t ah = (cpu->A() >> 4) - (val >> 4) - ((int8_t) al < 0);

  if ((uint8_t) diff == 0) {
    cpu->setZ();
  } else if (diff & 0x80) {
    cpu->setN();
  }

  if ((cpu->A() ^ val) & (cpu->A() ^ diff) & 0x80) {
    cpu->setV();
  }

  if (!(diff & 0xff00)) {
    cpu->setC();
  }
  if (ah & 0x80) {
    ah -= 6;
  }
  cpu->setA((ah << 4) | (al & 0x0f));
}

void do_sub_binary(M6502 *cpu, uint8_t val) {
  uint16_t diff = cpu->A() - val - (cpu->tstC() ? 0 : 1);

  cpu->clrV();
  cpu->clrC();

  if ((cpu->A() ^ val) & (cpu->A() ^ diff) & 0x80) {
    cpu->setV();
  }

  if (!(diff & 0xff00)) {
    cpu->setC();
  }

  cpu->setA(diff & 0xff);
  cpu->setNZ(cpu->A());
}

void do_sub(M6502 *cpu, uint8_t val) {
  if (cpu->tstD()) {
    do_sub_bcd(cpu, val);
  } else {
    do_sub_binary(cpu, val);
  }
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

uint8_t do_asl(M6502 *cpu, uint8_t val) {
  if (val & 0x80) cpu->setC(); else cpu->clrC();
  val = (val << 1) & 0xff;
  if (val & 0x80) cpu->setN(); else cpu->clrN();
  if (!val) cpu->setZ(); else cpu->clrZ();
  return val;
}

uint8_t do_lsr(M6502 *cpu, uint8_t val) {
  if (val & 0x01) cpu->setC(); else cpu->clrC();
  cpu->clrN();
  val = (val >> 1) & 0x7f;
  if (!val) cpu->setZ(); else cpu->clrZ();
  return val;
}

uint8_t do_rol(M6502 *cpu, uint8_t val) {
  bool tmp = (val & 0x80);
  val <<= 1;
  val |= cpu->tstC() ? 1 : 0;

  if (!val) cpu->setZ(); else cpu->clrZ();
  if (val & 0x80) cpu->setN(); else cpu->clrN();
  if (tmp) cpu->setC(); else cpu->clrC();
  return val;
}

uint8_t do_ror(M6502 *cpu, uint8_t val) {
  bool tmp = (val & 0x01);
  val = (val >> 1) & 0x7f;
  val |= cpu->tstC() ? 0x80 : 0;

  if (!val) cpu->setZ(); else cpu->clrZ();
  if (val & 0x80) cpu->setN(); else cpu->clrN();
  if (tmp) cpu->setC(); else cpu->clrC();
  return val;
}

/**
 * And with bits in accumulator  and set NZV accordingly
 * @param cpu For flags
 * @param val To test with acc
 */
void do_bit(M6502 *cpu, uint8_t val) {
  if (val & 0x80) cpu->setN(); else cpu->clrN();
  if (val & 0x40) cpu->setV(); else cpu->clrV();
  if (val & cpu->A()) cpu->clrZ();
  else cpu->setZ();
}

/**
 * Some operations take an extra cycle when a page crossing occurs
 * This checks for crossings and adjusts the cycle offset appropriately.
 */
void skip_cycle_on_page_crossing(M6502 *cpu, uint8_t offset) {
  if ((cpu->temp_addr_high() >> 8) == ((cpu->temp_addr() + offset) >> 8)) {
    // No page crossing, skip a cycle
    cpu->skip_cycle();
  }
}

void fetch(M6502 *cpu, Bus &bus) {
  bus.set_address(cpu->PC());
  bus.set_SYNC();
}

/*
 * Common branch steps
 */
void do_branch_0(M6502 *cpu, Bus &bus) { bus.set_address(cpu->incPC()); }

void do_branch_1(M6502 *cpu, Bus &bus, uint8_t flag, bool branch_if_set) {
  bus.set_address(cpu->PC());
  cpu->set_temp_addr(cpu->PC() + (int8_t) bus.get_data());

  // Can we avoid the branch?
  bool flag_set = (cpu->flags() & flag);
  if (flag_set == branch_if_set) return;

  // Skip branch
  fetch(cpu, bus);
}

void do_branch_2(M6502 *cpu, Bus &bus) {
  // Put un-page wrapped address on the bus
  bus.set_address((cpu->PC() & 0xff00) | (cpu->temp_addr_low()));
  if ((cpu->temp_addr_high()) == (cpu->PC() & 0xff00)) {
    cpu->setPC(cpu->temp_addr());
//        cpu->irq_pip >>= 1;
//        cpu->nmi_pip >>= 1;
    fetch(cpu, bus);
  }
}

void do_branch_3(M6502 *cpu, Bus &bus) {
  cpu->setPC(cpu->temp_addr());
  fetch(cpu, bus);
}

const std::map<uint16_t, CycleHandler> cycle_handlers = {
        /*******************
         *       BRK       *
         *******************/
        {0x000, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x001, [](M6502 *cpu, Bus &bus) {
          // If breaks are disabled
          if ((cpu->brk_flags_ & (BRK_IRQ | BRK_NMI))==0) {
            cpu->incPC();
          }

          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->PC() >> 8);

          // Only push data if we're not doing a RES
          if (!(cpu->brk_flags_ & BRK_RST)) {
            bus.clr_RW();
          }
        }},
        {0x002, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->PC());
          // Only push data if we're not doing a RES
          if (!(cpu->brk_flags_ & BRK_RST)) {
            bus.clr_RW();
          }
        }},
        {0x003, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->flags() | FLAG_X);

          // If this is a reset, read the RST vector to populate PC
          if (cpu->brk_flags_ & BRK_RST) {
            cpu->set_temp_addr(0xfffc);
          } else {
            bus.clr_RW();
            if (cpu->brk_flags_ & BRK_NMI) {
              // Set up NMI vector
              cpu->set_temp_addr(0xfffa);
            } else {
              // Set up with IRQ vector
              cpu->set_temp_addr(0xfffe);
            }
          }
        }},
        {0x004, [](M6502 *cpu, Bus &bus) {
          auto ta = cpu->temp_addr();
          bus.set_address(ta);
          cpu->set_temp_addr(ta + 1);

          cpu->setI();
          cpu->setB();
          cpu->brk_flags_ = 0;
          /* RES/NMI hijacking */
        }},
        {0x005, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x006, [](M6502 *cpu, Bus &bus) {
          cpu->setPC(bus.get_data() << 8 | cpu->temp_addr_low());
          fetch(cpu, bus);
        }},


        /*******************
         *   ORA (zp, X)   *
         *******************/
        {0x010, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x011, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x012, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0x013, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x014, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0x015, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ORA zp        *
         *******************/
        {0x050, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x051, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x052, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ASL zp        *
         *******************/
        {0x060, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x061, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x062, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x063, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_asl(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x064, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   PHP           *
         *******************/
        {0x080, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x081, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->flags() | FLAG_X);
          bus.clr_RW();
        }},
        {0x082, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   ORA #         *
         *******************/
        {0x090, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x091, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ASL A         *
         *******************/
        {0x0a0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x0a1, [](M6502 *cpu, Bus &bus) {
          cpu->setA(do_asl(cpu, cpu->A()));
          fetch(cpu, bus);
        }},


        /*******************
         *   ORA abs      *
         *******************/
        {0x0d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x0d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x0d2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x0d3, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ASL abs       *
         *******************/
        {0x0e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x0e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x0e2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x0e3, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x0e4, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_asl(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x0e5, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BPL #         *
         *******************/
        {0x100, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0x101, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_N, false);
        }},
        {0x102, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0x103, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},


        /*******************
         *   ORA (zp),Y)   *
         *******************/
        {0x110, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x111, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x112, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x113, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x114, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x115, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ORA zp,X      *
         *******************/
        {0x150, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x151, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x152, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x153, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ASL zp,X      *
         *******************/
        {0x160, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x161, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x162, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x163, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x164, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_asl(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x165, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},


        /*******************
         *   CLC           *
         *******************/
        {0x180, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x181, [](M6502 *cpu, Bus &bus) {
          cpu->clrC();
          fetch(cpu, bus);
        }},

        /*******************
         *   ORA abs,Y     *
         *******************/
        {0x190, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x191, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x192, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x193, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x194, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},


        /*******************
         *   ORA abs,X     *
         *******************/
        {0x1d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x1d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x1d2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x1d3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x1d4, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() | bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},


        /*******************
         *   ASL abs,X     *
         *******************/
        {0x1e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x1e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x1e2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x1e3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x1e4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x1e5, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_asl(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x1e6, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},


        /*******************
         *   JSR           *
         *******************/
        {0x200, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x201, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->SP());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x202, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->PC() >> 8);
          bus.clr_RW();
        }},
        {0x203, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->PC());
          bus.clr_RW();
        }},
        {0x204, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x205, [](M6502 *cpu, Bus &bus) {
          cpu->setPC(bus.get_data() << 8 | cpu->temp_addr_low());
          fetch(cpu, bus);
        }},


        /*******************
         *   AND (zp,X)    *
         *******************/
        {0x210, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x211, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x212, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0x213, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x214, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0x215, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   BIT zp        *
         *******************/
        {0x240, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x241, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x242, [](M6502 *cpu, Bus &bus) {
          do_bit(cpu, bus.get_data());
          fetch(cpu, bus);
        }},


        /*******************
         *   AND zp        *
         *******************/
        {0x250, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x251, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x252, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROL zp        *
         *******************/
        {0x260, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x261, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x262, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x263, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_rol(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x264, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   PLP           *
         *******************/
        {0x280, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x281, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->SP());
        }},
        {0x282, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
        }},
        {0x283, [](M6502 *cpu, Bus &bus) {
          cpu->set_flags((bus.get_data() | FLAG_B) & ~FLAG_X);
          fetch(cpu, bus);
        }},

        /*******************
         *   AND #         *
         *******************/
        {0x290, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x291, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},


        /*******************
         *   ROL A         *
         *******************/
        {0x2a0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x2a1, [](M6502 *cpu, Bus &bus) {
          cpu->setA(do_rol(cpu, cpu->A()));
          fetch(cpu, bus);
        }},


        /*******************
         *   BIT abs       *
         *******************/
        {0x2c0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x2c1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x2c2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x2c3, [](M6502 *cpu, Bus &bus) {
          do_bit(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   AND abs       *
         *******************/
        {0x2d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x2d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x2d2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x2d3, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROL abs       *
         *******************/
        {0x2e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x2e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x2e2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x2e3, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x2e4, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_rol(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x2e5, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BMI rel       *
         *******************/
        {0x300, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0x301, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_N, true);
        }},
        {0x302, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0x303, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},

        /*******************
         *   AND (zp),Y    *
         *******************/
        {0x310, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x311, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x312, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x313, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x314, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x315, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   AND zp,X      *
         *******************/
        {0x350, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x351, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x352, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x353, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROL zp,X      *
         *******************/
        {0x360, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x361, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x362, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x363, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x364, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_rol(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x365, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   SEC           *
         *******************/
        {0x380, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x381, [](M6502 *cpu, Bus &bus) {
          cpu->setC();
          fetch(cpu, bus);
        }},

        /*******************
         *   AND abs,Y     *
         *******************/
        {0x390, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x391, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x392, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x393, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x394, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   AND abs,X     *
         *******************/
        {0x3d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x3d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x3d2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x3d3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x3d4, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() & bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROL abs,X     *
         *******************/
        {0x3e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x3e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x3e2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x3e3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x3e4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x3e5, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_rol(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x3e6, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   RTI           *
         *******************/
        {0x400, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x401, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->SP());
        }},
        {0x402, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
        }},
        {0x403, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
          cpu->set_flags((bus.get_data() | FLAG_B) & ~FLAG_X);
        }},
        {0x404, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x405, [](M6502 *cpu, Bus &bus) {
          cpu->setPC(bus.get_data() << 8 | cpu->temp_addr_low());
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR zp,X      *
         *******************/
        {0x410, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x411, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x412, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0x413, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x414, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0x415, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR zp        *
         *******************/
        {0x450, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x451, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x452, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LSR zp        *
         *******************/
        {0x460, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x461, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x462, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x463, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_lsr(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x464, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   PHA           *
         *******************/
        {0x480, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x481, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->decSP());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x482, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR #         *
         *******************/
        {0x490, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x491, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LSR A         *
         *******************/
        {0x4a0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x4a1, [](M6502 *cpu, Bus &bus) {
          cpu->setA(do_lsr(cpu, cpu->A()));
          fetch(cpu, bus);
        }},

        /*******************
         *   JMP           *
         *******************/
        {0x4c0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x4c1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x4c2, [](M6502 *cpu, Bus &bus) {
          cpu->setPC(bus.get_data() << 8 | cpu->temp_addr_low());
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR abs       *
         *******************/
        {0x4d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x4d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x4d2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x4d3, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LSR abs       *
         *******************/
        {0x4e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x4e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x4e2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x4e3, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x4e4, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_lsr(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x4e5, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BVC rel       *
         *******************/
        {0x500, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0x501, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_V, false);
        }},
        {0x502, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0x503, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},

        /*******************
         *   EOR (zp),Y    *
         *******************/
        {0x510, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x511, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x512, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x513, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x514, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x515, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR zp,X      *
         *******************/
        {0x550, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x551, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x552, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x553, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LSR zp,X      *
         *******************/
        {0x560, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x561, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x562, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x563, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x564, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_lsr(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x565, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   CLI           *
         *******************/
        {0x580, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x581, [](M6502 *cpu, Bus &bus) {
          cpu->clrI();
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR abs,Y     *
         *******************/
        {0x590, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x591, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x592, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x593, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x594, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   EOR abs,X     *
         *******************/
        {0x5d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x5d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x5d2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x5d3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x5d4, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->A() ^ bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LSR abs,X     *
         *******************/
        {0x5e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x5e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x5e2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x5e3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x5e4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x5e5, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_lsr(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x5e6, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   RTS           *
         *******************/
        {0x600, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x601, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->SP());
        }},
        {0x602, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
        }},
        {0x603, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x604, [](M6502 *cpu, Bus &bus) {
          cpu->setPC(bus.get_data() << 8 | cpu->temp_addr_low());
          bus.set_address(cpu->incPC());
        }},
        {0x605, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   ADC (zp,X)    *
         *******************/
        {0x610, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x611, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x612, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0x613, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x614, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0x615, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ADC zp        *
         *******************/
        {0x650, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x651, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x652, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROR zp        *
         *******************/
        {0x660, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x661, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0x662, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x663, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_ror(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x664, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   PLA           *
         *******************/
        {0x680, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x681, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->SP());
        }},
        {0x682, [](M6502 *cpu, Bus &bus) {
          bus.set_address(0x100 | cpu->incSP());
        }},
        {0x683, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   ADC #         *
         *******************/
        {0x690, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x691, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROR A         *
         *******************/
        {0x6a0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x6a1, [](M6502 *cpu, Bus &bus) {
          cpu->setA(do_ror(cpu, cpu->A()));
          fetch(cpu, bus);
        }},

        /*******************
         *   JMP I         *
         *******************/
        {0x6c0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x6c1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x6c2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr());
        }},
        {0x6c3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr_high() |
                          ((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x6c4, [](M6502 *cpu, Bus &bus) {
          cpu->setPC(bus.get_data() << 8 | cpu->temp_addr_low());
          fetch(cpu, bus);
        }},


        /*******************
         *   ADC abs       *
         *******************/
        {0x6d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x6d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x6d2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x6d3, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROR abs       *
         *******************/
        {0x6e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x6e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x6e2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0x6e3, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x6e4, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_ror(cpu, cpu->temp_addr_low()));
          bus.clr_RW();
        }},
        {0x6e5, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BVS rel       *
         *******************/
        {0x700, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0x701, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_V, true);
        }},
        {0x702, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0x703, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},

        /*******************
         *   ADC (zp),Y    *
         *******************/
        {0x710, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x711, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x712, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x713, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x714, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x715, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ADC zp,X      *
         *******************/
        {0x750, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x751, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x752, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x753, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROR zp,X      *
         *******************/
        {0x760, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x761, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x762, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0x763, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x764, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_ror(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x765, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   SEI           *
         *******************/
        {0x780, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x781, [](M6502 *cpu, Bus &bus) {
          cpu->setI();
          fetch(cpu, bus);
        }},

        /*******************
         *   ADC abs,Y     *
         *******************/
        {0x790, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x791, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x792, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0x793, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0x794, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ADC abs,X     *
         *******************/
        {0x7d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x7d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x7d2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x7d3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x7d4, [](M6502 *cpu, Bus &bus) {
          do_add(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   ROR abs,X     *
         *******************/
        {0x7e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x7e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x7e2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0x7e3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0x7e4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0x7e5, [](M6502 *cpu, Bus &bus) {
          bus.set_data(do_ror(cpu, cpu->temp_addr()));
          bus.clr_RW();
        }},
        {0x7e6, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},


        /*******************
         *   STA zp,X      *
         *******************/
        {0x810, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x811, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x812, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0x813, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x814, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x815, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STY zp        *
         *******************/
        {0x840, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x841, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
          bus.set_data(cpu->Y());
          bus.clr_RW();
        }},
        {0x842, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STA zp        *
         *******************/
        {0x850, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x851, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x852, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STX zp        *
         *******************/
        {0x860, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x861, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
          bus.set_data(cpu->X());
          bus.clr_RW();
        }},
        {0x862, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   DEY           *
         *******************/
        {0x880, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x881, [](M6502 *cpu, Bus &bus) {
          cpu->decY();
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},

        /*******************
         *   TXA           *
         *******************/
        {0x8a0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x8a1, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->X());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   STY abs       *
         *******************/
        {0x8c0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x8c1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x8c2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
          bus.set_data(cpu->Y());
          bus.clr_RW();
        }},
        {0x8c3, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STA abs       *
         *******************/
        {0x8d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x8d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x8d2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x8d3, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STX abs       *
         *******************/
        {0x8e0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x8e1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x8e2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
          bus.set_data(cpu->X());
          bus.clr_RW();
        }},
        {0x8e3, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BCC rel       *
         *******************/
        {0x900, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0x901, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_C, false);
        }},
        {0x902, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0x903, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},

        /*******************
         *   STA (zp),Y    *
         *******************/
        {0x910, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x911, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x912, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x913, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
        }},
        {0x914, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x915, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STY zp,X      *
         *******************/
        {0x940, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x941, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x942, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_data(cpu->Y());
          bus.clr_RW();
        }},
        {0x943, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STA zp,X      *
         *******************/
        {0x950, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x951, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x952, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x953, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   STX zp,Y      *
         *******************/
        {0x960, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x961, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0x962, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->Y()) & 0xff);
          bus.set_data(cpu->X());
          bus.clr_RW();
        }},
        {0x963, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   TYA           *
         *******************/
        {0x980, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x981, [](M6502 *cpu, Bus &bus) {
          cpu->setA(cpu->Y());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   STA abs,Y     *
         *******************/
        {0x990, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x991, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x992, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
        }},
        {0x993, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x994, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   TXS           *
         *******************/
        {0x9a0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0x9a1, [](M6502 *cpu, Bus &bus) {
          cpu->setSP(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   STA abs,X     *
         *******************/
        {0x9d0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0x9d1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0x9d2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
        }},
        {0x9d3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
          bus.set_data(cpu->A());
          bus.clr_RW();
        }},
        {0x9d4, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   LDY #         *
         *******************/
        {0xa00, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa01, [](M6502 *cpu, Bus &bus) {
          cpu->setY(bus.get_data());
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDA (zp,X)    *
         *******************/
        {0xa10, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa11, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xa12, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0xa13, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xa14, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0xa15, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDX #         *
         *******************/
        {0xa20, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa21, [](M6502 *cpu, Bus &bus) {
          cpu->setX(bus.get_data());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},


        /*******************
         *   LDY zp        *
         *******************/
        {0xa40, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa41, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xa42, [](M6502 *cpu, Bus &bus) {
          cpu->setY(bus.get_data());
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDA zp        *
         *******************/
        {0xa50, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa51, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xa52, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDX zp        *
         *******************/
        {0xa60, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa61, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xa62, [](M6502 *cpu, Bus &bus) {
          cpu->setX(bus.get_data());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   TAY           *
         *******************/
        {0xa80, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xa81, [](M6502 *cpu, Bus &bus) {
          cpu->setY(cpu->A());
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},


        /*******************
         *   LDA #         *
         *******************/
        {0xa90, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xa91, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   TAX           *
         *******************/
        {0xaa0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xaa1, [](M6502 *cpu, Bus &bus) {
          cpu->setX(cpu->A());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDY abs       *
         *******************/
        {0xac0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xac1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xac2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xac3, [](M6502 *cpu, Bus &bus) {
          cpu->setY(bus.get_data());
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},


        /*******************
         *   LDA abs       *
         *******************/
        {0xad0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xad1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xad2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xad3, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDX abs       *
         *******************/
        {0xae0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xae1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xae2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xae3, [](M6502 *cpu, Bus &bus) {
          cpu->setX(bus.get_data());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   BCS #         *
         *******************/
        {0xb00, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0xb01, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_C, true);
        }},
        {0xb02, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0xb03, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},


        /*******************
         *   LDA (zp),Y    *
         *******************/
        {0xb10, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xb11, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xb12, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xb13, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xb14, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xb15, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDY zp,X      *
         *******************/
        {0xb40, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xb41, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xb42, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0xb43, [](M6502 *cpu, Bus &bus) {
          cpu->setY(bus.get_data());
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDA zp,X      *
         *******************/
        {0xb50, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xb51, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xb52, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0xb53, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDX zp,Y      *
         *******************/
        {0xb60, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xb61, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xb62, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->Y()) & 0xff);
        }},
        {0xb63, [](M6502 *cpu, Bus &bus) {
          cpu->setX(bus.get_data());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   CLV           *
         *******************/
        {0xb80, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xb81, [](M6502 *cpu, Bus &bus) {
          cpu->clrV();
          fetch(cpu, bus);
        }},

        /*******************
         *   LDA abs,Y     *
         *******************/
        {0xb90, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xb91, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xb92, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xb93, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xb94, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   TSX           *
         *******************/
        {0xba0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xba1, [](M6502 *cpu, Bus &bus) {
          cpu->setX(cpu->SP());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDY abs,X     *
         *******************/
        {0xbc0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xbc1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xbc2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0xbc3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0xbc4, [](M6502 *cpu, Bus &bus) {
          cpu->setY(bus.get_data());
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDA abs,X     *
         *******************/
        {0xbd0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xbd1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xbd2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0xbd3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0xbd4, [](M6502 *cpu, Bus &bus) {
          cpu->setA(bus.get_data());
          cpu->setNZ(cpu->A());
          fetch(cpu, bus);
        }},

        /*******************
         *   LDX abs,Y     *
         *******************/
        {0xbe0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xbe1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xbe2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xbe3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xbe4, [](M6502 *cpu, Bus &bus) {
          cpu->setX(bus.get_data());
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   CPY #         *
         *******************/
        {0xc00, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xc01, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->Y(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP (zp,X)    *
         *******************/
        {0xc10, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xc11, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xc12, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0xc13, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xc14, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0xc15, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   CPY zp        *
         *******************/
        {0xc40, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xc41, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xc42, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->Y(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP zp        *
         *******************/
        {0xc50, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xc51, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xc52, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   DEC zp        *
         *******************/
        {0xc60, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xc61, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xc62, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xc63, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() - 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xc64, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   INY           *
         *******************/
        {0xc80, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xc81, [](M6502 *cpu, Bus &bus) {
          cpu->incY();
          cpu->setNZ(cpu->Y());
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP #         *
         *******************/
        {0xc90, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xc91, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   DEX           *
         *******************/
        {0xca0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xca1, [](M6502 *cpu, Bus &bus) {
          cpu->decX();
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   CPY abs       *
         *******************/
        {0xcc0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xcc1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xcc2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xcc3, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->Y(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP abs       *
         *******************/
        {0xcd0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xcd1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xcd2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xcd3, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   DEC abs       *
         *******************/
        {0xce0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xce1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xce2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xce3, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xce4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() - 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xce5, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BNE #         *
         *******************/
        {0xd00, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0xd01, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_Z, false);
        }},
        {0xd02, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0xd03, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},

        /*******************
         *   CMP (zp),Y    *
         *******************/
        {0xd10, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xd11, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xd12, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xd13, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xd14, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xd15, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP zp,X      *
         *******************/
        {0xd50, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xd51, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xd52, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0xd53, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   DEC zp,X      *
         *******************/
        {0xd60, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xd61, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xd62, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0xd63, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xd64, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() - 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xd65, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   CLD           *
         *******************/
        {0xd80, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xd81, [](M6502 *cpu, Bus &bus) {
          cpu->clrD();
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP abs,Y     *
         *******************/
        {0xd90, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xd91, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xd92, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xd93, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xd94, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   CMP abs,X     *
         *******************/
        {0xdd0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xdd1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xdd2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0xdd3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0xdd4, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->A(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   DEC abs,X     *
         *******************/
        {0xde0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xde1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xde2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0xde3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0xde4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xde5, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() - 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xde6, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   CPX #         *
         *******************/
        {0xe00, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xe01, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->X(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC (zp,X)    *
         *******************/
        {0xE10, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xE11, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xE12, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr((cpu->temp_addr() + cpu->X()) & 0xff);
          bus.set_address(cpu->temp_addr());
        }},
        {0xE13, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + 1) & 0xff);
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xE14, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr());
        }},
        {0xE15, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},


        /*******************
         *   CPX zp        *
         *******************/
        {0xe40, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xe41, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xe42, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->X(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC zp        *
         *******************/
        {0xe50, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xe51, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xe52, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},


        /*******************
         *   INC zp        *
         *******************/
        {0xe60, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xe61, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data());
        }},
        {0xe62, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xe63, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() + 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xe64, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   INX           *
         *******************/
        {0xe80, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xe81, [](M6502 *cpu, Bus &bus) {
          cpu->incX();
          cpu->setNZ(cpu->X());
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC #         *
         *******************/
        {0xE90, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xE91, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},


        /*******************
         *   NOP           *
         *******************/
        {0xea0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xea1, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   CPX abs       *
         *******************/
        {0xec0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xec1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xec2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xec3, [](M6502 *cpu, Bus &bus) {
          do_cmp(cpu, cpu->X(), bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC abs       *
         *******************/
        {0xed0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xed1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xed2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xed3, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},


        /*******************
         *   INC abs       *
         *******************/
        {0xee0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xee1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xee2, [](M6502 *cpu, Bus &bus) {
          bus.set_address(bus.get_data() << 8 | cpu->temp_addr_low());
        }},
        {0xee3, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xee4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() + 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xee5, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   BEQ rel       *
         *******************/
        {0xf00, [](M6502 *cpu, Bus &bus) {
          do_branch_0(cpu, bus);
        }},
        {0xf01, [](M6502 *cpu, Bus &bus) {
          do_branch_1(cpu, bus, FLAG_Z, true);
        }},
        {0xf02, [](M6502 *cpu, Bus &bus) {
          do_branch_2(cpu, bus);
        }},
        {0xf03, [](M6502 *cpu, Bus &bus) {
          do_branch_3(cpu, bus);
        }},

        /*******************
         *   SBC (zp),Y    *
         *******************/
        {0xf10, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xf11, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xf12, [](M6502 *cpu, Bus &bus) {
          bus.set_address(((cpu->temp_addr_low() + 1) & 0xff));
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xf13, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xf14, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xf15, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC zp,X      *
         *******************/
        {0xf50, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xf51, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xf52, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0xf53, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},


        /*******************
         *   INC zp,X      *
         *******************/
        {0xf60, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xf61, [](M6502 *cpu, Bus &bus) {
          auto ta = bus.get_data();
          cpu->set_temp_addr(ta);
          bus.set_address(ta);
        }},
        {0xf62, [](M6502 *cpu, Bus &bus) {
          bus.set_address((cpu->temp_addr() + cpu->X()) & 0xff);
        }},
        {0xf63, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xf64, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() + 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xf65, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }},

        /*******************
         *   SED           *
         *******************/
        {0xf80, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->PC());
        }},
        {0xf81, [](M6502 *cpu, Bus &bus) {
          cpu->setD();
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC abs,Y     *
         *******************/
        {0xf90, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xf91, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xf92, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->Y()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->Y());
        }},
        {0xf93, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->Y());
        }},
        {0xf94, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   SBC abs,X     *
         *******************/
        {0xfd0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xfd1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xfd2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0xfd3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0xfd4, [](M6502 *cpu, Bus &bus) {
          do_sub(cpu, bus.get_data());
          fetch(cpu, bus);
        }},

        /*******************
         *   INC abs,X     *
         *******************/
        {0xfe0, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
        }},
        {0xfe1, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->incPC());
          cpu->set_temp_addr(bus.get_data());
        }},
        {0xfe2, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr_high(bus.get_data());
          bus.set_address(cpu->temp_addr_high() | ((cpu->temp_addr_low() + cpu->X()) & 0xff));
          skip_cycle_on_page_crossing(cpu, cpu->X());
        }},
        {0xfe3, [](M6502 *cpu, Bus &bus) {
          bus.set_address(cpu->temp_addr() + cpu->X());
        }},
        {0xfe4, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(bus.get_data());
          bus.clr_RW();
        }},
        {0xfe5, [](M6502 *cpu, Bus &bus) {
          cpu->set_temp_addr(cpu->temp_addr_low() + 1);
          cpu->setNZ(cpu->temp_addr_low());
          bus.set_data(cpu->temp_addr());
          bus.clr_RW();
        }},
        {0xfe6, [](M6502 *cpu, Bus &bus) {
          fetch(cpu, bus);
        }
        },
};

// Populate the flat lookup table from the map once at startup.
// Static init order within this TU is top-to-bottom, so cycle_handlers
// is fully constructed before table_ready runs.
static bool init_handler_table() {
  for (auto const &p : cycle_handlers) {
    handler_table[p.first] = p.second;
  }
  return true;
}
static bool table_ready = init_handler_table();