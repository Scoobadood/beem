#include "addressing.h"
#include "m6502.h"
#include <vector>

/**
 * Retrieve an absolute argument
 */
uint32_t get_arg_immediate(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  read_addr = cpu.pc_;
  return memory.at(cpu.pc_++);
}
AddressingFunction ImmediateData = get_arg_immediate;

uint32_t get_addr_zpg(M6502 &cpu, Memory &memory, bool &page_wrap) {
  page_wrap = false;
  return  memory.at(cpu.pc_++) & 0xff;
}
AddressComputeFunction ZeroPageAddress = get_addr_zpg;

/**
 * Retrieve a zero page argument
 */
uint32_t get_arg_zpg(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_zpg(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction ZeroPageData = get_arg_zpg;

uint32_t get_addr_zpg_x(M6502 &cpu, Memory &memory, bool &page_wrap) {
  page_wrap = false;
  uint32_t read_addr = memory.at(cpu.pc_++);
  return (read_addr + cpu.x_reg_) & 0xff;
}
AddressComputeFunction ZeroPageIndexedXAddress = get_addr_zpg_x;

/**
 * Retrieve an indexed zero page argument
 */
uint32_t get_arg_zpg_x(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_zpg_x(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction ZeroPageIndexedXData = get_arg_zpg_x;


uint32_t get_addr_zpg_y(M6502 &cpu, Memory &memory, bool &page_wrap) {
  page_wrap = false;
  uint32_t read_addr = memory.at(cpu.pc_++);
  return (read_addr + cpu.y_reg_) & 0xff;
}
AddressComputeFunction ZeroPageIndexedYAddress = get_addr_zpg_y;

/**
 * Retrieve an indexed zero page argument
 */
uint32_t get_arg_zpg_y(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_zpg_y(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction ZeroPageIndexedYData = get_arg_zpg_y;

uint32_t get_addr_absolute(M6502 &cpu, Memory &memory, bool &page_wrap) {
  page_wrap = false;
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  return ((addr_hi * 256) + addr_lo) & 0xffff;
}
AddressComputeFunction AbsoluteAddress = get_addr_absolute;

uint32_t get_arg_absolute(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_absolute(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction AbsoluteData = get_arg_absolute;


/*
 * This form of addressing is used in conjunction with the X index register.
 * The effective address is formed by adding the contents of X to the address
 * contained in the second and third bytes of the instruction.
 * This mode allows the index register to contain the index or count value and
 * the instruction to contain the base address. This type of indexing allows
 * any location referencing and the index to modify multiple fields resulting
 * in reduced coding and execution time.
 */
uint32_t get_addr_absolute_x(M6502 &cpu, Memory &memory, bool &page_wrap) {
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  uint32_t read_addr = (addr_hi * 256) + addr_lo;
  read_addr = (read_addr + cpu.x_reg_  ) & 0xffff;

  page_wrap = (((read_addr >> 8) & 0xff) != addr_hi);
  return read_addr;
}
AddressComputeFunction AbsoluteIndexedXAddress = get_addr_absolute_x;

uint32_t get_arg_absolute_x(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_absolute_x(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction AbsoluteIndexedXData = get_arg_absolute_x;

uint32_t get_addr_absolute_y(M6502 &cpu, Memory &memory, bool &page_wrap) {
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  uint32_t read_addr = (addr_hi * 256) + addr_lo;
  read_addr += cpu.y_reg_;
  page_wrap = (((read_addr >> 8) & 0xff) != addr_hi);
  return read_addr;
}
AddressComputeFunction AbsoluteIndexedYAddress = get_addr_absolute_y;

uint32_t get_arg_absolute_y(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_absolute_y(cpu,memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction AbsoluteIndexedYData = get_arg_absolute_y;

/**
 * Retrieve an indirect absolute ($abcd), Used with JMP
 */
uint32_t get_arg_indirect_absolute(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  page_wrap = false;
  uint8_t addr_lo = memory.at(cpu.pc_++);
  uint8_t addr_hi = memory.at(cpu.pc_++);
  read_addr = (addr_hi * 256) + addr_lo;
  addr_lo = memory.at(read_addr);
  addr_hi = memory.at(read_addr+ 1);
  read_addr = (addr_hi * 256) + addr_lo;
  return read_addr;
}
AddressingFunction IndirectAbsoluteData = get_arg_indirect_absolute;


uint32_t get_addr_indexed_indirect(M6502 &cpu, Memory &memory, bool &page_wrap) {
  page_wrap = false;
  uint8_t zpg = memory.at(cpu.pc_++);
  zpg += cpu.x_reg_;
  uint8_t addr_lo = memory.at(zpg);
  uint8_t addr_hi = memory.at((zpg + 1) & 0xff);
  return ((addr_hi * 256) + addr_lo) & 0xffff;
}
AddressComputeFunction IndexedIndirectAddress = get_addr_indexed_indirect;

/**
 * LDA ($B4, X)
 */
uint32_t get_arg_indexed_indirect(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_indexed_indirect(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction IndexedIndirectData = get_arg_indexed_indirect;

/**
 * LDA ($B4), X
 */

uint32_t get_addr_indirect_indexed(M6502 &cpu, Memory &memory, bool &page_wrap) {
  uint8_t zpg = memory.at(cpu.pc_++);
  uint8_t addr_lo = memory.at(zpg);
  uint8_t addr_hi = memory.at((zpg + 1) & 0xff);
  uint32_t read_addr = (addr_hi * 256) + addr_lo;
  read_addr += cpu.y_reg_;
  page_wrap = (((read_addr >> 8) & 0xff) != addr_hi);
  return read_addr;
}
AddressComputeFunction IndirectIndexedAddress = get_addr_indirect_indexed;

uint32_t get_arg_indirect_indexed(M6502 &cpu, Memory &memory, uint32_t & read_addr, bool &page_wrap) {
  read_addr = get_addr_indirect_indexed(cpu, memory, page_wrap);
  return memory.at(read_addr);
}
AddressingFunction IndirectIndexedData = get_arg_indirect_indexed;
