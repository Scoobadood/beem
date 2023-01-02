#include "addressing.h"
#include "cpu.h"
#include <vector>

/**
 * Retrieve aan absolute argument
 */
uint32_t get_arg_immediate(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  read_addr = cpu.pc_;
  return memory.at(cpu.pc_++);
}
AddressingFunction Immediate = get_arg_immediate;

/**
 * Retrieve a zero page argument
 */
uint32_t get_arg_zpg(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  read_addr = memory.at(cpu.pc_++);
  return memory.at(read_addr);
}
AddressingFunction ZeroPage = get_arg_zpg;

/**
 * Retrieve an indexed zero page argument
 */
uint32_t get_arg_zpg_x(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  read_addr = memory.at(cpu.pc_++);
  read_addr = (read_addr + cpu.x_reg_) & 0xff;
  return memory.at(read_addr);
}
AddressingFunction ZeroPageIndexedX = get_arg_zpg_x;

/**
 * Retrieve an indexed zero page argument
 */
uint32_t get_arg_zpg_y(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  read_addr = memory.at(cpu.pc_++);
  read_addr = (read_addr + cpu.y_reg_) & 0xff;
  return memory.at(read_addr);
}
AddressingFunction ZeroPageIndexedY = get_arg_zpg_y;

uint32_t get_arg_absolute(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  read_addr = ((addr_hi * 256) + addr_lo) & 0xffff;
  return memory.at(read_addr);
}
AddressingFunction Absolute = get_arg_absolute;

/*
 * This form of addressing is used in conjunction with the X index register.
 * The effective address is formed by adding the contents of X to the address
 * contained in the second and third bytes of the instruction.
 * This mode allows the index register to contain the index or count value and
 * the instruction to contain the base address. This type of indexing allows
 * any location referencing and the index to modify multiple fields resulting
 * in reduced coding and execution time.
 */
uint32_t get_arg_absolute_x(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  read_addr = (addr_hi * 256) + addr_lo;
  read_addr = (read_addr + cpu.x_reg_  ) & 0xffff;

  page_wrap = (((read_addr >> 8) & 0xff) != addr_hi);
  return memory.at(read_addr);
}
AddressingFunction AbsoluteIndexedX = get_arg_absolute_x;

uint32_t get_arg_absolute_y(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  read_addr = (addr_hi * 256) + addr_lo;
  read_addr += cpu.y_reg_;
  page_wrap = (((read_addr >> 8) & 0xff) != addr_hi);
  return memory.at(read_addr);
}
AddressingFunction AbsoluteIndexedY = get_arg_absolute_y;

/**
 * Retrieve an indirect absolute ($abcd), Used with JMP
 */
uint32_t get_arg_indirect_absolute(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  read_addr = (addr_hi * 256) + addr_lo;
  addr_lo = memory.at(read_addr);
  addr_hi = memory.at(read_addr+ 1);
  read_addr = (addr_hi * 256) + addr_lo;
  return read_addr;
}
AddressingFunction IndirectAbsolute = get_arg_indirect_absolute;

/**
 * LDA ($B4, X)
 */
uint32_t get_arg_indexed_indirect(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  uint8_t zpg = memory.at(cpu.pc_++);
  zpg += cpu.x_reg_;
  uint8_t addr_lo = memory.at(zpg);
  uint8_t addr_hi = memory.at((zpg + 1) & 0xff);
  read_addr = (addr_hi * 256) + addr_lo;
  return memory.at(read_addr);
}
AddressingFunction IndexedIndirect = get_arg_indexed_indirect;

/**
 * LDA ($B4), X
 */
uint32_t get_arg_indirect_indexed(Cpu &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  uint8_t zpg = memory.at(cpu.pc_++);
  uint8_t addr_lo = memory.at(zpg);
  uint8_t addr_hi = memory.at((zpg + 1) & 0xff);
  read_addr = (addr_hi * 256) + addr_lo;
  read_addr += cpu.y_reg_;
  page_wrap = (((read_addr >> 8) & 0xff) != addr_hi);

  return memory.at(read_addr);
}
AddressingFunction IndirectIndexed = get_arg_indirect_indexed;
