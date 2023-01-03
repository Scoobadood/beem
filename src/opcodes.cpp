//
// Created by Dave Durbin on 1/12/2022.
//

#include "opcodes.h"
#include "addressing.h"
#include "memory.h"

#include <map>
#include <sstream>
#include <iomanip>

#undef DEBUG_ADC
#undef DEBUG_SBC

#if defined(DEBUG_ADC) || defined(DEBUG_SBC)
#include <iostream>
#include <iomanip>
#endif

const uint8_t RES_FLAG = 0x20;
const uint8_t BRK_FLAG = 0x10;

/*
 * ADC
 * Add Memory to Accumulator with Carry
 * A + M + C -> A, C
 *
 * N  Z  C  I  D  V
 * +  +	 +  -  -  +
 *
 * addressing	assembler	opc	bytes	cycles
 * immediate	ADC #oper	 69	 2   	2
 * zeropage	    ADC oper     65	 2	    3
 * zeropage,X	ADC oper,X	 75	 2	    4
 * absolute	    ADC oper	 6D	 3 	    4
 * absolute,X	ADC oper,X	 7D	 3 	    4*
 * absolute,Y	ADC oper,Y	 79	 3	    4*
 * (indirect,X)	ADC (oper,X) 61	 2	    6
 * (indirect),Y	ADC (oper),Y 71	 2	    5*
 */


// Implementation taken from http://www.6502.org/tutorials/decimal_mode.html
void adc_decimal(Cpu &cpu, uint32_t arg) {
  using namespace std;

#ifdef DEBUG_ADC
  cout << "add_dec "
       << hex << setw(2) << setfill('0') << cpu.accumulator_
       << " + "
       << hex << setw(2) << setfill('0') << arg
       << " + " << (cpu.carry() ? 1 : 0);
#endif

//  1a. AL = (A & $0F) + (B & $0F) + C
  uint32_t carry = cpu.carry() ? 1 : 0;
  auto al = (cpu.accumulator_ & 0x0f) + (arg & 0x0f) + carry;

//  1b. If AL >= $0A, then AL = ((AL + $06) & $0F) + $10
  if (al >= 0x0a) al = ((al + 6) & 0xf) + 0x10;

//  1c. A = (A & $F0) + (B & $F0) + AL
  auto a = (cpu.accumulator_ & 0xf0) + (arg & 0xf0) + al;

//  1d. Note that A can be >= $100 at this point
//  1e. If (A >= $A0), then A = A + $60
  if (a >= 0xA0) a += 0x60;

//  1f. The accumulator result is the lower 8 bits of A
  cpu.accumulator_ = a & 0xff;

//  1g. The carry result is 1 if A >= $100, and is 0 if A < $100
  cpu.status_.set(SR_CRY, a >= 0x100);
  cpu.status_.set(SR_ZER, (a & 0xff) == 0);
  cpu.status_.set(SR_NEG, (a & 0x80));

#ifdef DEBUG_ADC
  cout << " = "
       << hex << setw(2) << setfill('0') << cpu.accumulator_
       << "   c:" << (cpu.carry() ? 1 : 0)
       << std::endl;
#endif
}

void adc(Cpu &cpu, uint32_t arg) {
  if (cpu.is_decimal()) {
    adc_decimal(cpu, arg);
    return;
  }
  auto result = cpu.accumulator_;
  result += (arg & 0xff);
  result += (cpu.carry() ? 1 : 0);
  cpu.status_.set(SR_CRY, (result > 0xff));

  result &= 0xff;
  cpu.status_.set(SR_NEG, (result & 0x80));
  cpu.status_.set(SR_ZER, (result == 0));

  cpu.status_.set(SR_OVF, !((cpu.accumulator_ ^ arg) & 0x80) && ((cpu.accumulator_ ^ result) & 0x80));

  cpu.accumulator_ = result;
}

void adc_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void adc_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}
void adc_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}
void adc_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}
void adc_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}
void adc_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}
void adc_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}
void adc_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * AND Memory with Accumulator

  A AND M -> A
  N	Z	C	I	D	V
  +	+	-	-	-	-
  addressing	assembler     opc	bytes	cycles
  immediate	    AND #oper	  29	2     	2
  zeropage	    AND oper	  25	2	    3
  zeropage,X	AND oper,X	  35	2	    4
  absolute	    AND oper	  2D	3	    4
  absolute,X	AND oper,X	  3D	3	    4*
  absolute,Y	AND oper,Y	  39	3	    4*
  (indirect,X)	AND (oper,X)  21	2	    6
  (indirect),Y	AND (oper),Y  31	2	    5*
 */

void anda(Cpu &cpu, uint32_t arg) {
  cpu.accumulator_ &= (arg & 0xff);
  cpu.status_.set(SR_NEG, (cpu.accumulator_ & 0x80));
  cpu.status_.set(SR_ZER, (cpu.accumulator_ == 0));
}

void and_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void and_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void and_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void and_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void and_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void and_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void and_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}

void and_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * ASL
 * Shift Left One Bit (Memory or Accumulator)
 *
 * C <- [76543210] <- 0
 *
 * N	Z	C	I	D	V
 * +	+	+	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * accumulator	ASL A	    0A	1	2
 * zeropage	ASL oper	06	2	5
 * zeropage,X	ASL oper,X	16	2	6
 * absolute	ASL oper	0E	3	6
 * absolute,X	ASL oper,X	1E	3	7
 */
uint8_t asl(Cpu &cpu, uint32_t arg) {
  cpu.status_.set(SR_CRY, (arg & 0x80));

  arg = (arg << 1) & 0xff;
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  return (arg & 0xff);
}

void asl_a(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.accumulator_ = asl(cpu, cpu.accumulator_);
  clk += 2;
}

void asl_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageData(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.set(addr, arg);
  clk += 5;
}

void asl_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedXData(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.set(addr, arg);
  clk += 6;
}

void asl_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteData(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.set(addr, arg);
  clk += 6;
}

void asl_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedXData(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.set(addr, arg);
  clk += 7;
}

void do_branch(Cpu &cpu, Memory &memory, int8_t branch, uint64_t &clk) {
  // Branch taken, inc clk.
  clk++;

  auto old_addr_hi = (cpu.pc_ >> 8) & 0xff;
  cpu.pc_ += branch;
  auto new_addr_hi = (cpu.pc_ >> 8) & 0xff;
  if (old_addr_hi != new_addr_hi) {
    clk++;
  }
}

/*
 * BCC
 * Branch on Carry Clear
 *
 * branch on C = 0
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * relative	    BCC oper	90	2	    2**
 */
void bcc(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (cpu.carry_clear()) {
    do_branch(cpu, memory, branch, clk);
  }
}

/*
 * Branch on Carry Set
 *
 * branch on C = 1
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * relative	    BCS oper	B0	2	    2**
 */
void bcs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (cpu.carry()) {
    do_branch(cpu, memory, branch, clk);
  }
}

/*
 * BEQ
 * Branch if EQual
 *
 * branch on Z = 1
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * relative	    BEQ oper	    2	    2**
 */
void beq(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (cpu.zero()) {
    do_branch(cpu, memory, branch, clk);
  }
}

/*
 * BIT
 *
 * Test Bits in Memory with Accumulator
 * bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
 * the zero-flag is set to the result of operand AND accumulator.
 *
 * A AND M, M7 -> N, M6 -> V
 * N	Z	C	I	D	V
 * M7	+	-	-	-	M6
 * addressing	assembler	opc	bytes	cycles
 * zeropage	    BIT oper	24	2	3
 * absolute	    BIT oper	2C	3	4
 */
void bit(Cpu &cpu, uint8_t arg) {
  cpu.status_.set(SR_NEG, (arg & (1 << SR_NEG)));
  cpu.status_.set(SR_OVF, (arg & (1 << SR_OVF)));
  cpu.status_.set(SR_ZER, !(arg & cpu.accumulator_));
}

void bit_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  bit(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void bit_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  bit(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void bmi(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (cpu.minus()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void bne(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (cpu.not_zero()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void bpl(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = (int8_t) memory.at(cpu.pc_++);
  if (cpu.plus()) {
    do_branch(cpu, memory, branch, clk);
  }
}

/*
 * BRK
 * Force Break
 *
 * BRK initiates a software interrupt similar to a hardware interrupt (IRQ).
 * The return address pushed to the stack is PC+2, providing an extra byte of spacing
 * for a break mark (identifying a reason for the break.)
 *
 * The status register will be pushed to the stack with the break
 * flag set to 1. However, when retrieved during RTI or by a PLP instruction, the break flag will be ignored.
 *
 * The interrupt disable flag is not set automatically.
 *
 * interrupt,
 * push PC+2, push SR
 * N	Z	C	I	D	V
 * -	-	-	1	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	BRK	00	1	7
 */
void brek(Cpu &cpu, Memory &memory, uint64_t &clk) {
  // PC currently points to the reson byte after BRK
  auto pc = cpu.pc_ + 1;
  memory.push_stack(cpu, pc >> 8);
  memory.push_stack(cpu, pc & 0xff);

  auto status = (cpu.status_.to_ulong());
  status |= BRK_FLAG;
  status |= RES_FLAG;
  memory.push_stack(cpu, status);

  cpu.set_interrupt();

  cpu.pc_ = (memory.at(0xfffe) + (memory.at(0xffff) * 256)) & 0xffff;
  clk += 7;
}

void bvc(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (!cpu.is_overflow()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void bvs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  auto branch = int8_t(memory.at(cpu.pc_++));
  if (cpu.is_overflow()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void clc(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  cpu.clear_carry();
}

void cld(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  cpu.clear_decimal();
}

void cli(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  cpu.clear_interrupt();
}

void clv(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
  cpu.clear_overflow();
}

/*
 * CMP
 *
 * Compare Memory with Accumulator
 *
 * A - M
 * N	Z	C	I	D	V
 * +	+	+	-	-	-
 * addressing	assembler	  opc	bytes cycles
 * immediate	CMP #oper	  C9	2	  2
 * zeropage	    CMP oper	  C5	2	  3
 * zeropage,X	CMP oper,X	  D5	2	  4
 * absolute	    CMP oper	  CD	3	  4
 * absolute,X	CMP oper,X	  DD	3	  4*
 * absolute,Y	CMP oper,Y	  D9	3	  4*
 * (indirect,X)	CMP (oper,X)  C1	2	  6
 * (indirect),Y	CMP (oper),Y  D1	2	  5*
 */

/*
 * This instruction subtracts the contents of memory from the contents of the accumulator.
 *
 * The use of the CMP affects the following flags:
 * Z flag is set on an equal comparison, reset otherwise;
 * the N flag is set or reset by the result bit 7,
 * the carry flag is set when the value in memory is less than or equal to the accumulator,
 * reset when it is greater than the accumulator. The accumulator is not affected.
 * +---------------------+---+---+---+
 * | Compare Result	     | N | Z | C |
 * +---------------------+---+---+---+
 * | A, X, or Y < Memory | * | 0 | 0 |
 * | A, X, or Y = Memory | 0 | 1 | 1 |
 * | A, X, or Y > Memory | * | 0 | 1 |
 * +---------------------+---+---+---+
 * *The N flag will be bit 7 of A, X, or Y - Memory
 */
void cmp(Cpu &cpu, uint8_t arg) {
  auto t = (cpu.accumulator_ - arg) & 0xff;
  cpu.status_.set(SR_NEG, (t & 0x80));
  cpu.status_.set(SR_ZER, (t == 0));
  cpu.status_.set(SR_CRY, (arg <= cpu.accumulator_));
}

void cmp_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void cmp_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void cmp_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void cmp_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void cmp_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void cmp_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void cmp_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}

void cmp_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * CPX
 *
 * Compare Memory and Index X
 * X - M
 * N	Z	C	I	D	V
 * +	+	+	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * immediate	CPX #oper	E0	2	2
 * zeropage	    CPX oper	E4	2	3
 * absolute	    CPX oper	EC	3	4
 */
void cpx(Cpu &cpu, uint8_t arg) {
  auto t = (cpu.x_reg_ - arg) & 0xff;
  cpu.status_.set(SR_NEG, (t & 0x80));
  cpu.status_.set(SR_ZER, (t == 0));
  cpu.status_.set(SR_CRY, (arg <= cpu.x_reg_));
}

void cpx_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpx(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void cpx_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpx(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void cpx_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpx(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

/*
 * CPY
 *
 * Compare Memory and Index Y
 * Y - M
 * N	Z	C	I	D	V
 * +	+	+	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * immediate	CPY #oper	C0	2	2
 * zeropage	CPY oper	C4	2	3
 * absolute	CPY oper	CC	3	4
*/
void cpy(Cpu &cpu, uint8_t arg) {
  auto t = (cpu.y_reg_ - arg) & 0xff;
  cpu.status_.set(SR_NEG, (t & 0x80));
  cpu.status_.set(SR_ZER, (t == 0));
  cpu.status_.set(SR_CRY, (arg <= cpu.y_reg_));
}

void cpy_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpy(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void cpy_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpy(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void cpy_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpy(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

/*
 * DEC
 * Decrement Memory by One
 *
 * M - 1 -> M
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * zeropage	    DEC oper	C6	2	5
 * zeropage,X	DEC oper,X	D6	2	6
 * absolute	    DEC oper	CE	3	6
 * absolute,X	DEC oper,X	DE	3	7
 */
uint8_t dec(Cpu &cpu, uint8_t arg) {
  arg = (arg - 1) & 0xff;
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  return arg;
}

void dec_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 5;
}

void dec_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 6;
}

void dec_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 6;
}

void dec_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 7;
}

/*
 * DEX
 * Decrement Index X by One
 *
 * X - 1 -> X
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 *
 * addressing	assembler	opc	bytes	cycles
 * implied	    DEX	        CA	  1	    2
*/
void dex(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.x_reg_ = (cpu.x_reg_ - 1) & 0xff;
  cpu.status_.set(SR_NEG, (cpu.x_reg_ & 0x80));
  cpu.status_.set(SR_ZER, (cpu.x_reg_ == 0));
  clk += 2;
}

/*
 * DEY
 * Decrement Index Y by One
 *
 * Y - 1 -> Y
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	DEY	88	1	2
*/
void dey(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.y_reg_ = (cpu.y_reg_ - 1) & 0xff;
  cpu.status_.set(SR_NEG, (cpu.y_reg_ & 0x80));
  cpu.status_.set(SR_ZER, (cpu.y_reg_ == 0));
  clk += 2;
}

/*
 * EOR
 *
 * Exclusive-OR Memory with Accumulator
 *
 * A EOR M -> A
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * immediate	EOR #oper	  49	2	2
 * zeropage	    EOR oper	  45	2	3
 * zeropage,X	EOR oper,X	  55	2	4
 * absolute	    EOR oper	  4D	3	4
 * absolute,X	EOR oper,X	  5D	3	4*
 * absolute,Y	EOR oper,Y	  59	3	4*
 * (indirect,X)	EOR (oper,X)  41	2	6
 * (indirect),Y	EOR (oper),Y  51	2	5*
 */
void eor(Cpu &cpu, uint8_t arg) {
  cpu.accumulator_ = (cpu.accumulator_ ^ arg);
  cpu.status_.set(SR_NEG, (cpu.accumulator_ & 0x80));
  cpu.status_.set(SR_ZER, (cpu.accumulator_ == 0));
}

void eor_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void eor_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void eor_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void eor_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void eor_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void eor_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void eor_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}

void eor_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * INC
 * Increment Memory by One
 *
 * M + 1 -> M
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * zeropage	INC oper	E6	2	5
 * zeropage,X	INC oper,X	F6	2	6
 * absolute	    INC oper	EE	3	6
 * absolute,X	INC oper,X	FE	3	7
 */
uint8_t inc(Cpu &cpu, uint8_t arg) {
  arg = (arg + 1) & 0xff;
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  return arg;
}

void inc_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 5;
}

void inc_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 6;
}

void inc_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 6;
}

void inc_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  memory.set(addr, ans);
  clk += 7;
}

/*
 * INX
 * Increment Index X by One
 *
 * X + 1 -> X
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	INX	E8	1	2
 */
void inx(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.x_reg_ = (cpu.x_reg_ + 1) & 0xff;
  if (cpu.x_reg_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.x_reg_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

/*
 * INY
 * Increment Index Y by One
 *
 * Y + 1 -> Y
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	INY	C8	1	2
 */
void iny(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.y_reg_ = (cpu.y_reg_ + 1) & 0xff;
  if (cpu.y_reg_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.y_reg_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

/*
 * JMP
 * Jump to New Location
 *
 * (PC+1) -> PCL
 * (PC+2) -> PCH
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 *
 * addressing	assembler	opc	bytes	cycles
 * absolute	JMP oper	4C	3	3
 * indirect	JMP (oper)	6C	3	5
 */
void jmp(Cpu &cpu, uint8_t lo, uint8_t hi) {
  auto addr = ((hi * 256) + lo) & 0xffff;
  cpu.pc_ = addr;
}

void jmp_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  auto pcl = memory.at(cpu.pc_++);
  auto pch = memory.at(cpu.pc_++);
  jmp(cpu, pcl, pch);
  clk += 3;
}

/*
 * The indirect jump instruction does not increment the page address when the
 * indirect pointer crosses a page boundary.
 * JMP ($xxFF) will fetch the address from $xxFF and $xx00.
 */
void jmp_ind(Cpu &cpu, Memory &memory, uint64_t &clk) {
  auto al = memory.at(cpu.pc_++);
  auto ah = memory.at(cpu.pc_++);
  auto addr = ((ah * 256) + al) & 0xffff;
  auto pcl = memory.at(addr);
  if (al == 0xff) {
    addr = (ah * 256);
  } else {
    addr += 1;
  }
  auto pch = memory.at(addr);
  jmp(cpu, pcl, pch);
  clk += 5;
}

/*
 * Jump to New Location Saving Return Address
 *
 * push (PC+2),
 * (PC+1) -> PCL
 * (PC+2) -> PCH
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 *
 * addressing	assembler	opc	bytes cycles
 * absolute	    JSR oper	20	  3	  6
 */
void jsr(Cpu &cpu, Memory &memory, uint64_t &clk) {
  // PC currently points at first byte of arg
  auto ret_addr = cpu.pc_ + 1;

  memory.push_stack(cpu, ret_addr >> 8);
  memory.push_stack(cpu, ret_addr & 0xff);
  auto pcl = memory.at(cpu.pc_++);
  auto pch = memory.at(cpu.pc_++);
  auto addr = ((pch * 256) + pcl) & 0xffff;
  cpu.pc_ = addr;
  clk += 6;
}

/*
 * LDA
 * Load Accumulator with Memory
 * M -> A
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	  opc	bytes	cycles
 * immediate	LDA #oper	  A9	2	2
 * zeropage	    LDA oper	  A5	2	3
 * zeropage,X	LDA oper,X	  B5	2	4
 * absolute	    LDA oper	  AD	3	4
 * absolute,X	LDA oper,X	  BD	3	4*
 * absolute,Y	LDA oper,Y	  B9	3	4*
 * (indirect,X)	LDA (oper,X)  A1	2	6
 * (indirect),Y	LDA (oper),Y  B1	2	5*
 */

void lda(Cpu &cpu, uint32_t arg) {
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  cpu.accumulator_ = arg;
}

void lda_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void lda_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void lda_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void lda_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void lda_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void lda_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void lda_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}

void lda_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * Load Index X with Memory
 *
 * M -> X
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * immediate	LDX #oper	A2	2	2
 * zeropage	    LDX oper	A6	2	3
 * zeropage,Y	LDX oper,Y	B6	2	4
 * absolute	    LDX oper	AE	3	4
 * absolute,Y	LDX oper,Y	BE	3	4*
 */
void ldx(Cpu &cpu, uint32_t arg) {
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  cpu.x_reg_ = (arg & 0xff);
}

void ldx_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void ldx_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void ldx_zpg_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, ZeroPageIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldx_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldx_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

/*
 * LDY
 * Load Index Y with Memory
 * M -> Y
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * immediate	LDY #oper	A0	2	2
 * zeropage	    LDY oper	A4	2	3
 * zeropage,X	LDY oper,X	B4	2	4
 * absolute	    LDY oper	AC	3	4
 * absolute,X	LDY oper,X	BC	3	4*
 */
void ldy(Cpu &cpu, uint32_t arg) {
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  cpu.y_reg_ = (arg & 0xff);
}

void ldy_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void ldy_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void ldy_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldy_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldy_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

/*
 * LSR
 *
 * Shift One Bit Right (Memory or Accumulator)
 * 0 -> [76543210] -> C
 *
 * N	Z	C	I	D	V
 * 0	+	+	-	-	-
 *
 * addressing	assembler	opc	bytes	cycles
 * accumulator	LSR A	    4A	1	2
 * zeropage	    LSR oper	46	2	5
 * zeropage,X	LSR oper,X	56	2	6
 * absolute	    LSR oper	4E	3	6
 * absolute,X	LSR oper,X	5E	3	7
 */
uint8_t lsr(Cpu &cpu, uint32_t arg) {
  cpu.status_.set(SR_CRY, (arg & 0x01));
  arg = (arg >> 1) & 0x7f;
  cpu.status_.set(SR_NEG, false);
  cpu.status_.set(SR_ZER, (arg == 0));
  return arg;
}

void lsr_a(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.accumulator_ = lsr(cpu, cpu.accumulator_);
  clk += 2;
}

void lsr_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageData(cpu, memory, addr, page_wrap);
  memory.set(addr, lsr(cpu, arg));
  clk += 5;
}

void lsr_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedXData(cpu, memory, addr, page_wrap);
  memory.set(addr, lsr(cpu, arg));
  clk += 6;
}

void lsr_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteData(cpu, memory, addr, page_wrap);
  memory.set(addr, lsr(cpu, arg));
  clk += 6;
}

void lsr_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedXData(cpu, memory, addr, page_wrap);
  memory.set(addr, lsr(cpu, arg));
  clk += 7;
}

/*
 * No Operation
 * ---
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	    NOP	EA	    1        	2
 */
void nop(Cpu &cpu, Memory &memory, uint64_t &clk) {
  clk += 2;
}

/*
 * ORA
 *
 * OR Memory with Accumulator
 *
 * A OR M -> A
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 *
 * addressing	assembler	  opc	bytes	cycles
 * immediate	ORA #oper	  09	2	2
 * zeropage	    ORA oper	  05	2	3
 * zeropage,X	ORA oper,X	  15	2	4
 * absolute	    ORA oper	  0D	3	4
 * absolute,X	ORA oper,X	  1D	3	4*
 * absolute,Y	ORA oper,Y	  19	3	4*
 * (indirect,X)	ORA (oper,X)  01	2	6
 * (indirect),Y	ORA (oper),Y  11	2	5*
 */

void or_a(Cpu &cpu, uint32_t arg) {
  cpu.accumulator_ |= (arg & 0xff);
  if (cpu.accumulator_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.accumulator_ == 0) cpu.set_zero(); else cpu.clear_zero();
}

void ora_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void ora_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void ora_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ora_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ora_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void ora_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void ora_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}

void ora_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * PHA
 * Push Accumulator on Stack
 * push A
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	    PHA	        48	1	3
 */
void pha(Cpu &cpu, Memory &memory, uint64_t &clk) {
  memory.push_stack(cpu, cpu.accumulator_);
  clk += 3;
}

/*
 * PHP
 * Push Processor Status on Stack
 * The status register will be pushed with the break
 * flag and bit 5 set to 1.
 * push SR
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	PHP	08	1	3
 */
void php(Cpu &cpu, Memory &memory, uint64_t &clk) {
  auto arg = cpu.status_.to_ulong();
  arg |= BRK_FLAG;
  arg |= RES_FLAG;
  memory.push_stack(cpu, arg);

  clk += 3;
}

/*
 * PLA
 * Pull Accumulator from Stack
 * pull A
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	PLA	68	1	4
 */
void pla(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.accumulator_ = memory.pop_stack(cpu);
  cpu.status_.set(SR_NEG, (cpu.accumulator_ & 0x80));
  cpu.status_.set(SR_ZER, (cpu.accumulator_ == 0));
  clk += 4;
}

/*
 * PLP
 * Pull Processor Status from Stack
 * The status register will be pulled with the break
 * flag and bit 5 ignored.
 * pull SR
 * N	Z	C	I	D	V
 * from stack
 * addressing	assembler	opc	bytes	cycles
 * implied	PLP	28	1	4
 */
void plp(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.status_ = memory.pop_stack(cpu);
  clk += 4;
}

/*
 * ROL
 * Rotate One Bit Left (Memory or Accumulator)
 *
 * C <- [76543210] <- C
 * N	Z	C	I	D	V
 * +	+	+	-	-	-
 *
 * addressing	assembler	opc	bytes	cycles
 * accumulator	ROL A	    2A	1	2
 * zeropage	    ROL oper	26	2	5
 * zeropage,X	ROL oper,X	36	2	6
 * absolute	    ROL oper	2E	3	6
 * absolute,X	ROL oper,X	3E	3	7
 */
uint8_t rol(Cpu &cpu, uint32_t arg) {
  auto carry = cpu.carry() ? 0x01 : 0x00;
  if (arg & 0x80) cpu.set_carry(); else cpu.clear_carry();
  arg = ((arg << 1) | carry) & 0xff;

  if (arg == 0) cpu.set_zero(); else cpu.clear_zero();
  if (arg & 0x80) cpu.set_neg(); else cpu.clear_neg();
  return arg;
}

void rol_a(Cpu &cpu, Memory &memory, uint64_t &clk) {
  auto arg = cpu.accumulator_;
  arg = rol(cpu, arg);
  cpu.accumulator_ = arg;
  clk += 2;
}

void rol_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageData(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.set(addr, arg);
  clk += 5;
}

void rol_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedXData(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.set(addr, arg);
  clk += 6;
}

void rol_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteData(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.set(addr, arg);
  clk += 6;
}

void rol_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedXData(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.set(addr, arg);
  clk += 7;
}

/*
 * ROR
 * Rotate One Bit Right (Memory or Accumulator)
 * C -> [76543210] -> C
 * N	Z	C	I	D	V
 * +	+	+	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * accumulator	ROR A	    6A	1	2
 * zeropage	    ROR oper	66	2	5
 * zeropage,X	ROR oper,X	76	2	6
 * absolute	    ROR oper	6E	3	6
 * absolute,X	ROR oper,X	7E	3	7
 */

uint8_t ror(Cpu &cpu, uint32_t arg) {
  auto carry = cpu.carry() ? 0x80 : 0x00;
  if (arg & 0x01) cpu.set_carry(); else cpu.clear_carry();
  arg = ((arg >> 1) | carry) & 0xff;

  if (arg == 0) cpu.set_zero(); else cpu.clear_zero();
  if (arg & 0x80) cpu.set_neg(); else cpu.clear_neg();
  return arg;
}

void ror_a(Cpu &cpu, Memory &memory, uint64_t &clk) {
  auto arg = cpu.accumulator_;
  arg = ror(cpu, arg);
  cpu.accumulator_ = arg;
  clk += 2;
}

void ror_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageData(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.set(addr, arg);
  clk += 5;
}

void ror_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedXData(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.set(addr, arg);
  clk += 6;
}

void ror_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteData(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.set(addr, arg);
  clk += 6;
}

void ror_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedXData(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.set(addr, arg);
  clk += 7;
}

/*
 * RTI
 * Return from Interrupt
 * The status register is pulled with the break flag
 * and bit 5 ignored. Then PC is pulled from the stack.
 *
 * pull SR, pull PC
 * N	Z	C	I	D	V
 * from stack
 * addressing	assembler	opc	bytes	cycles
 * implied	RTI	40	1	6
*/

void rti(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.status_ = memory.pop_stack(cpu);

  auto pcl = memory.pop_stack(cpu);
  auto pch = memory.pop_stack(cpu);
  cpu.pc_ = ((pch * 256) + pcl) & 0xffff;
  clk += 6;
}

/*
 * RTS
 * Return from Subroutine
 *
 * pull PC, PC+1 -> PC
 *
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 *
 * addressing	assembler	opc	bytes	cycles
 * implied	    RTS	        60	1	6
 */
void rts(Cpu &cpu, Memory &memory, uint64_t &clk) {
  auto pcl = memory.pop_stack(cpu);
  auto pch = memory.pop_stack(cpu);
  cpu.pc_ = ((pch * 256) + pcl) & 0xffff;
  cpu.pc_++;
  clk += 6;
}

/*
 * SBC
 * Subtract Memory from Accumulator with Borrow
 *
 * A - M - C -> A
 * N	Z	C	I	D	V
 * +	+	+	-	-	+
 *
 * addressing	assembler	  opc	bytes	cycles
 * immediate	SBC #oper	  E9	2	2
 * zeropage	    SBC oper	  E5	2	3
 * zeropage,X	SBC oper,X	  F5	2	4
 * absolute	    SBC oper	  ED	3	4
 * absolute,X	SBC oper,X	  FD	3	4*
 * absolute,Y	SBC oper,Y	  F9	3	4*
 * (indirect,X)	SBC (oper,X)  E1	2	6
 * (indirect),Y	SBC (oper),Y  F1	2	5*
 */

// Per http://www.6502.org/tutorials/decimal_mode.html
void sbc_decimal(Cpu &cpu, uint32_t arg) {
  using namespace std;

#ifdef DEBUG_SBC
  cout << "sub_dec "
       << hex << setw(2) << setfill('0') << cpu.accumulator_
       << " - "
       << hex << setw(2) << setfill('0') << arg
       << " - " << (cpu.carry_clear() ? 1 : 0);
#endif



  //  3a. AL = (A & $0F) - (B & $0F) + C-1
  auto borrow = (cpu.carry_clear() ? 1 : 0);
  int32_t al = (cpu.accumulator_ & 0x0f) - (arg & 0x0f) - borrow;

  //  3b. If AL < 0, then AL = ((AL - $06) & $0F) - $10
  if (al < 0) al = ((al - 0x06) & 0x0f) - 0x10;

  //  3c. A = (A & $F0) - (B & $F0) + AL
  int32_t a = (cpu.accumulator_ & 0xf0) - (arg & 0xf0) + al;

  //  3d. If A < 0, then A = A - $60
  if (a < 0) a -= 0x60;

  cpu.status_.set(SR_CRY, cpu.accumulator_ >= (arg + borrow));

  //  3e. The accumulator result is the lower 8 bits of A
  cpu.accumulator_ = a & 0xff;

  /* The flags are set just like in Binary mode. */
  cpu.status_.set(SR_ZER, (cpu.accumulator_ & 0xff) == 0);
  cpu.status_.set(SR_NEG, (cpu.accumulator_ & 0x80));

#ifdef DEBUG_SBC
  cout << " = "
       << hex << setw(2) << setfill('0') << cpu.accumulator_
       << "   c:" << (cpu.carry() ? 1 : 0)
       << std::endl;
#endif
}

void sbc(Cpu &cpu, uint32_t arg) {
  if (cpu.is_decimal()) {
    sbc_decimal(cpu, arg);
    return;
  }

  // Per http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
  // Implement as adc with ones complement of arg
  adc(cpu, (0xff - arg));
}

void sbc_imm(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, ImmediateData(cpu, memory, addr, page_wrap));
  clk += 2;
}

void sbc_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, ZeroPageData(cpu, memory, addr, page_wrap));
  clk += 3;
}

void sbc_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, ZeroPageIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void sbc_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, AbsoluteData(cpu, memory, addr, page_wrap));
  clk += 4;
}

void sbc_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, AbsoluteIndexedXData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void sbc_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, AbsoluteIndexedYData(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void sbc_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, IndexedIndirectData(cpu, memory, addr, page_wrap));
  clk += 6;
}

void sbc_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, IndirectIndexedData(cpu, memory, addr, page_wrap));
  clk += 5;
  if (page_wrap) clk++;
}

/*
 * SEC
 * Set Carry Flag
 *
 * 1 -> C
 * N	Z	C	I	D	V
 * -	-	1	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	SEC	38	1	2
 */
void sec(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.set_carry();
  clk += 2;
}

/*
 * SED
 * Set Decimal Flag
 *
 * 1 -> D
 * N	Z	C	I	D	V
 * -	-	-	-	1	-
 * addressing	assembler	opc	bytes	cycles
 * implied	SED	F8	1	2
 */
void sed(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.set_decimal();
  clk += 2;
}

/*
 * SEI
 *
 * Set Interrupt Disable Status
 * 1 -> I
 * N	Z	C	I	D	V
 * -	-	-	1	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	SEI	78	1	2
 */
void sei(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.set_interrupt();
  clk += 2;
}

/*
 * STA
 * Store Accumulator in Memory
 *
 * A -> M
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 *
 * addressing	assembler     opc	bytes	cycles
 * zeropage	    STA oper	  85	2	3
 * zeropage,X	STA oper,X	  95	2	4
 * absolute	    STA oper	  8D	3	4
 * absolute,X	STA oper,X	  9D	3	5
 * absolute,Y	STA oper,Y	  99	3	5
 * (indirect,X)	STA (oper,X)  81	2	6
 * (indirect),Y	STA (oper),Y  91	2	6
 */
void sta_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = ZeroPageAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 3;
}

void sta_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = ZeroPageIndexedXAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 4;
}

void sta_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = AbsoluteAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 4;
}

void sta_abs_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = AbsoluteIndexedXAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 5;
}

void sta_abs_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = AbsoluteIndexedYAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 5;
}

void sta_ind_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = IndexedIndirectAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 6;
}

void sta_ind_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = IndirectIndexedAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.accumulator_);
  clk += 6;
}

/*
 * STX
 * Store Index X in Memory
 *
 * X -> M
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * zeropage	    STX oper	86	2	3
 * zeropage,Y	STX oper,Y	96	2	4
 * absolute	    STX oper	8E	3	4
 */
void stx_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = ZeroPageAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.x_reg_);
  clk += 3;
}
void stx_zpg_y(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = ZeroPageIndexedYAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.x_reg_);
  clk += 4;
}
void stx_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = AbsoluteAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.x_reg_);
  clk += 4;
}

/*
 * STY
 * Store Index Y in Memory
 *
 * Y -> M
 * N	Z	C	I	D	V
 * -	-	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * zeropage	    STY oper	84	2	3
 * zeropage,X	STY oper,X	94	2	4
 * absolute	    STY oper	8C	3	4
 */
void sty_zpg(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = ZeroPageAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.y_reg_);
  clk += 3;
}
void sty_zpg_x(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = ZeroPageIndexedXAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.y_reg_);
  clk += 4;
}
void sty_abs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  bool page_wrap;
  auto addr = AbsoluteAddress(cpu, memory, page_wrap);
  memory.set(addr, cpu.y_reg_);
  clk += 4;
}

//TAX
// Transfer Accumulator to Index X
//
//    A -> X
//    N	Z	C	I	D	V
//+	+	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    implied	TAX	AA	1	2
void tax(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.x_reg_ = cpu.accumulator_;
  if (cpu.x_reg_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.x_reg_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

//TAY
//    Transfer Accumulator to Index Y
//
//A -> Y
//    N	Z	C	I	D	V
//+	+	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    implied	TAY	A8	1	2
void tay(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.y_reg_ = cpu.accumulator_;
  if (cpu.y_reg_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.y_reg_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

/*
 * TSX
 * Transfer Stack Pointer to Index X
 *
 * SP -> X
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	    TSX	        BA	1	    2
 */
void tsx(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.x_reg_ = (cpu.stack_pointer_ & 0xff);
  cpu.status_.set(SR_NEG, cpu.x_reg_ & 0x80);
  cpu.status_.set(SR_ZER, cpu.x_reg_ == 0);
  clk += 2;
}

/*
 * TXA
 * Transfer Index X to Accumulator
 *
 * X -> A
 * N	Z	C	I	D	V
 * +	+	-	-	-	-
 * addressing	assembler	opc	bytes	cycles
 * implied	    TXA         8A  1       2
 */
void txa(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.accumulator_ = cpu.x_reg_;
  if (cpu.accumulator_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.accumulator_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

//TXS
//    Transfer Index X to Stack Register
//
//    X -> SP
//    N	Z	C	I	D	V
//-	-	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    implied	TXS	9A	1	2

void txs(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.stack_pointer_ = (cpu.x_reg_ & 0xff);
  clk += 2;
}

//TYA
//    Transfer Index Y to Accumulator
//
//Y -> A
//    N	Z	C	I	D	V
//+	+	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    implied	TYA	98	1	2
void tya(Cpu &cpu, Memory &memory, uint64_t &clk) {
  cpu.accumulator_ = cpu.y_reg_;
  if (cpu.accumulator_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.accumulator_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

const std::map<uint8_t, OpCode> codes = {
    {(uint8_t) 0x69, {2, 2, false, "adc", OpCode::AddressingMode::Immediate, adc_imm}},
    {(uint8_t) 0x65, {2, 3, false, "adc", OpCode::AddressingMode::ZeroPage, adc_zpg}},
    {(uint8_t) 0x75, {2, 4, false, "adc", OpCode::AddressingMode::ZeroPageIndexedX, adc_zpg_x}},
    {(uint8_t) 0x6d, {3, 4, false, "adc", OpCode::AddressingMode::Absolute, adc_abs}},
    {(uint8_t) 0x7d, {3, 4, true, "adc", OpCode::AddressingMode::AbsoluteIndexedX, adc_abs_x}},
    {(uint8_t) 0x79, {3, 4, true, "adc", OpCode::AddressingMode::AbsoluteIndexedY, adc_abs_y}},
    {(uint8_t) 0x61, {2, 6, false, "adc", OpCode::AddressingMode::IndirectIndexedX, adc_ind_x}},
    {(uint8_t) 0x71, {2, 5, false, "adc", OpCode::AddressingMode::IndirectIndexedY, adc_ind_y}},
    {(uint8_t) 0x29, {2, 2, false, "and", OpCode::AddressingMode::Immediate, and_imm}},
    {(uint8_t) 0x25, {2, 3, false, "and", OpCode::AddressingMode::ZeroPage, and_zpg}},
    {(uint8_t) 0x35, {2, 4, false, "and", OpCode::AddressingMode::ZeroPageIndexedX, and_zpg_x}},
    {(uint8_t) 0x2d, {3, 4, false, "and", OpCode::AddressingMode::Absolute, and_abs}},
    {(uint8_t) 0x3d, {3, 4, true, "and", OpCode::AddressingMode::AbsoluteIndexedX, and_abs_x}},
    {(uint8_t) 0x39, {3, 4, true, "and", OpCode::AddressingMode::AbsoluteIndexedY, and_abs_y}},
    {(uint8_t) 0x21, {2, 6, false, "and", OpCode::AddressingMode::IndirectIndexedX, and_ind_x}},
    {(uint8_t) 0x31, {2, 5, false, "and", OpCode::AddressingMode::IndirectIndexedY, and_ind_y}},
    {(uint8_t) 0x0a, {1, 2, false, "asl", OpCode::AddressingMode::Accumulator, asl_a}},
    {(uint8_t) 0x06, {2, 5, false, "asl", OpCode::AddressingMode::ZeroPage, asl_zpg}},
    {(uint8_t) 0x16, {2, 6, false, "asl", OpCode::AddressingMode::ZeroPageIndexedX, asl_zpg_x}},
    {(uint8_t) 0x0e, {3, 6, false, "asl", OpCode::AddressingMode::Absolute, asl_abs}},
    {(uint8_t) 0x1e, {3, 7, false, "asl", OpCode::AddressingMode::AbsoluteIndexedX, asl_abs_x}},
    {(uint8_t) 0x90, {2, 2, true, "bcc", OpCode::AddressingMode::Relative, bcc}},
    {(uint8_t) 0xb0, {2, 2, true, "bcs", OpCode::AddressingMode::Relative, bcs}},
    {(uint8_t) 0xf0, {2, 2, true, "beq", OpCode::AddressingMode::Relative, beq}},
    {(uint8_t) 0x30, {2, 2, true, "bmi", OpCode::AddressingMode::Relative, bmi}},
    {(uint8_t) 0xd0, {2, 2, true, "bne", OpCode::AddressingMode::Relative, bne}},
    {(uint8_t) 0x10, {2, 2, true, "bpl", OpCode::AddressingMode::Relative, bpl}},
    {(uint8_t) 0x50, {2, 2, true, "bvc", OpCode::AddressingMode::Relative, bvc}},
    {(uint8_t) 0x70, {2, 2, true, "bvs", OpCode::AddressingMode::Relative, bvs}},
    {(uint8_t) 0x00, {1, 7, false, "brk", OpCode::AddressingMode::Implied, brek}},
    {(uint8_t) 0x18, {1, 2, false, "clc", OpCode::AddressingMode::Implied, clc}},
    {(uint8_t) 0xd8, {1, 2, false, "cld", OpCode::AddressingMode::Implied, cld}},
    {(uint8_t) 0x58, {1, 2, false, "cli", OpCode::AddressingMode::Implied, cli}},
    {(uint8_t) 0xb8, {1, 2, false, "clv", OpCode::AddressingMode::Implied, clv}},
    {(uint8_t) 0x24, {2, 3, false, "bit", OpCode::AddressingMode::ZeroPage, bit_zpg}},
    {(uint8_t) 0x2c, {3, 4, false, "bit", OpCode::AddressingMode::Absolute, bit_abs}},
    {(uint8_t) 0xc9, {2, 2, false, "cmp", OpCode::AddressingMode::Immediate, cmp_imm}},
    {(uint8_t) 0xc5, {2, 3, false, "cmp", OpCode::AddressingMode::ZeroPage, cmp_zpg}},
    {(uint8_t) 0xd5, {2, 4, false, "cmp", OpCode::AddressingMode::ZeroPageIndexedX, cmp_zpg_x}},
    {(uint8_t) 0xcd, {3, 4, false, "cmp", OpCode::AddressingMode::Absolute, cmp_abs}},
    {(uint8_t) 0xdd, {3, 4, true, "cmp", OpCode::AddressingMode::AbsoluteIndexedX, cmp_abs_x}},
    {(uint8_t) 0xd9, {3, 4, true, "cmp", OpCode::AddressingMode::AbsoluteIndexedY, cmp_abs_y}},
    {(uint8_t) 0xc1, {2, 6, false, "cmp", OpCode::AddressingMode::IndirectIndexedX, cmp_ind_x}},
    {(uint8_t) 0xd1, {2, 5, false, "cmp", OpCode::AddressingMode::IndirectIndexedY, cmp_ind_y}},
    {(uint8_t) 0xe0, {2, 2, false, "cpx", OpCode::AddressingMode::Immediate, cpx_imm}},
    {(uint8_t) 0xe4, {2, 3, false, "cpx", OpCode::AddressingMode::ZeroPage, cpx_zpg}},
    {(uint8_t) 0xec, {3, 4, false, "cpx", OpCode::AddressingMode::Absolute, cpx_abs}},
    {(uint8_t) 0xc0, {2, 2, false, "cpy", OpCode::AddressingMode::Immediate, cpy_imm}},
    {(uint8_t) 0xc4, {2, 3, false, "cpy", OpCode::AddressingMode::ZeroPage, cpy_zpg}},
    {(uint8_t) 0xcc, {3, 4, false, "cpy", OpCode::AddressingMode::Absolute, cpy_abs}},
    {(uint8_t) 0xc6, {2, 5, false, "dec", OpCode::AddressingMode::ZeroPage, dec_zpg}},
    {(uint8_t) 0xd6, {2, 6, false, "dec", OpCode::AddressingMode::ZeroPageIndexedX, dec_zpg_x}},
    {(uint8_t) 0xce, {3, 6, false, "dec", OpCode::AddressingMode::Absolute, dec_abs}},
    {(uint8_t) 0xde, {3, 7, false, "dec", OpCode::AddressingMode::AbsoluteIndexedX, dec_abs_x}},
    {(uint8_t) 0xca, {1, 2, false, "dex", OpCode::AddressingMode::Implied, dex}},
    {(uint8_t) 0x88, {1, 2, false, "dey", OpCode::AddressingMode::Implied, dey}},
    {(uint8_t) 0x49, {2, 2, false, "eor", OpCode::AddressingMode::Immediate, eor_imm}},
    {(uint8_t) 0x45, {2, 3, false, "eor", OpCode::AddressingMode::ZeroPage, eor_zpg}},
    {(uint8_t) 0x55, {2, 4, false, "eor", OpCode::AddressingMode::ZeroPageIndexedX, eor_zpg_x}},
    {(uint8_t) 0x4d, {3, 4, false, "eor", OpCode::AddressingMode::Absolute, eor_abs}},
    {(uint8_t) 0x5d, {3, 4, true, "eor", OpCode::AddressingMode::AbsoluteIndexedX, eor_abs_x}},
    {(uint8_t) 0x59, {3, 4, true, "eor", OpCode::AddressingMode::AbsoluteIndexedY, eor_abs_y}},
    {(uint8_t) 0x41, {2, 6, false, "eor", OpCode::AddressingMode::IndirectIndexedX, eor_ind_x}},
    {(uint8_t) 0x51, {2, 5, true, "eor", OpCode::AddressingMode::IndirectIndexedY, eor_ind_y}},
    {(uint8_t) 0xe6, {2, 5, false, "inc", OpCode::AddressingMode::ZeroPage, inc_zpg}},
    {(uint8_t) 0xf6, {2, 6, false, "inc", OpCode::AddressingMode::ZeroPageIndexedX, inc_zpg_x}},
    {(uint8_t) 0xee, {3, 6, false, "inc", OpCode::AddressingMode::Absolute, inc_abs}},
    {(uint8_t) 0xfe, {3, 7, false, "inc", OpCode::AddressingMode::AbsoluteIndexedX, inc_abs_x}},
    {(uint8_t) 0xe8, {1, 2, false, "inx", OpCode::AddressingMode::Implied, inx}},
    {(uint8_t) 0xc8, {1, 2, false, "iny", OpCode::AddressingMode::Implied, iny}},
    {(uint8_t) 0x4c, {3, 3, false, "jmp", OpCode::AddressingMode::Absolute, jmp_abs}},
    {(uint8_t) 0x6c, {3, 5, false, "jmp", OpCode::AddressingMode::Indirect, jmp_ind}},
    {(uint8_t) 0x20, {3, 6, false, "jsr", OpCode::AddressingMode::Absolute, jsr}},
    {(uint8_t) 0xa5, {2, 2, false, "lda", OpCode::AddressingMode::ZeroPage, lda_zpg}},
    {(uint8_t) 0xa9, {2, 3, false, "lda", OpCode::AddressingMode::Immediate, lda_imm}},
    {(uint8_t) 0xad, {2, 4, false, "lda", OpCode::AddressingMode::Absolute, lda_abs}},
    {(uint8_t) 0xb5, {3, 4, false, "lda", OpCode::AddressingMode::ZeroPageIndexedX, lda_zpg_x}},
    {(uint8_t) 0xb9, {3, 4, true, "lda", OpCode::AddressingMode::AbsoluteIndexedY, lda_abs_y}},
    {(uint8_t) 0xbd, {3, 4, true, "lda", OpCode::AddressingMode::AbsoluteIndexedX, lda_abs_x}},
    {(uint8_t) 0xa1, {2, 6, false, "lda", OpCode::AddressingMode::IndirectIndexedX, lda_ind_x}},
    {(uint8_t) 0xb1, {2, 5, true, "lda", OpCode::AddressingMode::IndirectIndexedY, lda_ind_y}},
    {(uint8_t) 0xa2, {2, 2, false, "ldx", OpCode::AddressingMode::Immediate, ldx_imm}},
    {(uint8_t) 0xa6, {2, 3, false, "ldx", OpCode::AddressingMode::ZeroPage, ldx_zpg}},
    {(uint8_t) 0xb6, {2, 4, false, "ldx", OpCode::AddressingMode::ZeroPageIndexedY, ldx_zpg_y}},
    {(uint8_t) 0xae, {3, 4, false, "ldx", OpCode::AddressingMode::Absolute, ldx_abs}},
    {(uint8_t) 0xbe, {3, 4, true, "ldx", OpCode::AddressingMode::AbsoluteIndexedY, ldx_abs_y}},
    {(uint8_t) 0xa0, {2, 2, false, "ldy", OpCode::AddressingMode::Immediate, ldy_imm}},
    {(uint8_t) 0xa4, {2, 3, false, "ldy", OpCode::AddressingMode::ZeroPage, ldy_zpg}},
    {(uint8_t) 0xb4, {2, 4, false, "ldy", OpCode::AddressingMode::ZeroPageIndexedX, ldy_zpg_x}},
    {(uint8_t) 0xac, {3, 4, false, "ldy", OpCode::AddressingMode::Absolute, ldy_abs}},
    {(uint8_t) 0xbc, {3, 4, true, "ldy", OpCode::AddressingMode::AbsoluteIndexedX, ldy_abs_x}},
    {(uint8_t) 0x4a, {1, 2, false, "lsr", OpCode::AddressingMode::Accumulator, lsr_a}},
    {(uint8_t) 0x46, {2, 5, false, "lsr", OpCode::AddressingMode::ZeroPage, lsr_zpg}},
    {(uint8_t) 0x56, {2, 6, false, "lsr", OpCode::AddressingMode::ZeroPageIndexedX, lsr_zpg_x}},
    {(uint8_t) 0x4e, {3, 6, false, "lsr", OpCode::AddressingMode::Absolute, lsr_abs}},
    {(uint8_t) 0x5e, {3, 7, false, "lsr", OpCode::AddressingMode::AbsoluteIndexedX, lsr_abs_x}},
    {(uint8_t) 0xea, {1, 2, false, "nop", OpCode::AddressingMode::Implied, nop}},
    {(uint8_t) 0x09, {2, 2, false, "ora", OpCode::AddressingMode::Immediate, ora_imm}},
    {(uint8_t) 0x05, {2, 3, false, "ora", OpCode::AddressingMode::ZeroPage, ora_zpg}},
    {(uint8_t) 0x15, {2, 4, false, "ora", OpCode::AddressingMode::ZeroPageIndexedX, ora_zpg_x}},
    {(uint8_t) 0x0d, {3, 4, false, "ora", OpCode::AddressingMode::Absolute, ora_abs}},
    {(uint8_t) 0x1d, {3, 4, true, "ora", OpCode::AddressingMode::AbsoluteIndexedX, ora_abs_x}},
    {(uint8_t) 0x19, {3, 4, true, "ora", OpCode::AddressingMode::AbsoluteIndexedY, ora_abs_y}},
    {(uint8_t) 0x01, {2, 6, false, "ora", OpCode::AddressingMode::IndirectIndexedX, ora_ind_x}},
    {(uint8_t) 0x11, {2, 5, true, "ora", OpCode::AddressingMode::IndirectIndexedY, ora_ind_y}},
    {(uint8_t) 0x48, {1, 3, false, "pha", OpCode::AddressingMode::Implied, pha}},
    {(uint8_t) 0x08, {1, 3, false, "php", OpCode::AddressingMode::Implied, php}},
    {(uint8_t) 0x68, {1, 4, false, "pla", OpCode::AddressingMode::Implied, pla}},
    {(uint8_t) 0x28, {1, 4, false, "plp", OpCode::AddressingMode::Implied, plp}},
    {(uint8_t) 0x2a, {1, 2, false, "rol", OpCode::AddressingMode::Accumulator, rol_a}},
    {(uint8_t) 0x26, {2, 5, false, "rol", OpCode::AddressingMode::ZeroPage, rol_zpg}},
    {(uint8_t) 0x36, {2, 6, false, "rol", OpCode::AddressingMode::ZeroPageIndexedX, rol_zpg_x}},
    {(uint8_t) 0x2e, {3, 6, false, "rol", OpCode::AddressingMode::Absolute, rol_abs}},
    {(uint8_t) 0x3e, {3, 7, false, "rol", OpCode::AddressingMode::AbsoluteIndexedX, rol_abs_x}},
    {(uint8_t) 0x6a, {1, 2, false, "ror", OpCode::AddressingMode::Accumulator, ror_a}},
    {(uint8_t) 0x66, {2, 5, false, "ror", OpCode::AddressingMode::ZeroPage, ror_zpg}},
    {(uint8_t) 0x76, {2, 6, false, "ror", OpCode::AddressingMode::ZeroPageIndexedX, ror_zpg_x}},
    {(uint8_t) 0x6e, {3, 6, false, "ror", OpCode::AddressingMode::Absolute, ror_abs}},
    {(uint8_t) 0x7e, {3, 7, false, "ror", OpCode::AddressingMode::AbsoluteIndexedX, ror_abs_x}},
    {(uint8_t) 0x40, {1, 6, false, "rti", OpCode::AddressingMode::Implied, rti}},
    {(uint8_t) 0x60, {1, 6, false, "rts", OpCode::AddressingMode::Implied, rts}},
    {(uint8_t) 0xe9, {2, 2, false, "sbc", OpCode::AddressingMode::Immediate, sbc_imm}},
    {(uint8_t) 0xe5, {2, 3, false, "sbc", OpCode::AddressingMode::ZeroPage, sbc_zpg}},
    {(uint8_t) 0xf5, {2, 4, false, "sbc", OpCode::AddressingMode::ZeroPageIndexedX, sbc_zpg_x}},
    {(uint8_t) 0xed, {3, 4, false, "sbc", OpCode::AddressingMode::Absolute, sbc_abs}},
    {(uint8_t) 0xfd, {3, 4, true, "sbc", OpCode::AddressingMode::AbsoluteIndexedX, sbc_abs_x}},
    {(uint8_t) 0xf9, {3, 4, true, "sbc", OpCode::AddressingMode::AbsoluteIndexedY, sbc_abs_y}},
    {(uint8_t) 0xe1, {2, 6, false, "sbc", OpCode::AddressingMode::IndirectIndexedX, sbc_ind_x}},
    {(uint8_t) 0xf1, {2, 5, true, "sbc", OpCode::AddressingMode::IndirectIndexedY, sbc_ind_y}},
    {(uint8_t) 0x38, {1, 2, false, "sec", OpCode::AddressingMode::Implied, sec}},
    {(uint8_t) 0xf8, {1, 2, false, "sed", OpCode::AddressingMode::Implied, sed}},
    {(uint8_t) 0x78, {1, 2, false, "sei", OpCode::AddressingMode::Implied, sei}},
    {(uint8_t) 0x85, {2, 3, false, "sta", OpCode::AddressingMode::ZeroPage, sta_zpg}},
    {(uint8_t) 0x95, {2, 4, false, "sta", OpCode::AddressingMode::ZeroPageIndexedX, sta_zpg_x}},
    {(uint8_t) 0x8d, {3, 4, false, "sta", OpCode::AddressingMode::Absolute, sta_abs}},
    {(uint8_t) 0x9d, {3, 5, false, "sta", OpCode::AddressingMode::AbsoluteIndexedX, sta_abs_x}},
    {(uint8_t) 0x99, {3, 5, false, "sta", OpCode::AddressingMode::AbsoluteIndexedY, sta_abs_y}},
    {(uint8_t) 0x81, {2, 6, false, "sta", OpCode::AddressingMode::IndirectIndexedX, sta_ind_x}},
    {(uint8_t) 0x91, {2, 6, false, "sta", OpCode::AddressingMode::IndirectIndexedY, sta_ind_y}},
    {(uint8_t) 0x86, {2, 3, false, "stx", OpCode::AddressingMode::ZeroPage, stx_zpg}},
    {(uint8_t) 0x96, {2, 4, false, "stx", OpCode::AddressingMode::ZeroPageIndexedY, stx_zpg_y}},
    {(uint8_t) 0x8e, {3, 4, false, "stx", OpCode::AddressingMode::Absolute, stx_abs}},
    {(uint8_t) 0x84, {2, 3, false, "sty", OpCode::AddressingMode::ZeroPage, sty_zpg}},
    {(uint8_t) 0x94, {2, 4, false, "sty", OpCode::AddressingMode::ZeroPageIndexedX, sty_zpg_x}},
    {(uint8_t) 0x8c, {3, 4, false, "sty", OpCode::AddressingMode::Absolute, sty_abs}},
    {(uint8_t) 0xaa, {1, 2, false, "tax", OpCode::AddressingMode::Implied, tax}},
    {(uint8_t) 0xa8, {1, 2, false, "tay", OpCode::AddressingMode::Implied, tay}},
    {(uint8_t) 0xba, {1, 2, false, "tsx", OpCode::AddressingMode::Implied, tsx}},
    {(uint8_t) 0x8a, {1, 2, false, "txa", OpCode::AddressingMode::Implied, txa}},
    {(uint8_t) 0x9a, {1, 2, false, "txs", OpCode::AddressingMode::Implied, txs}},
    {(uint8_t) 0x98, {1, 2, false, "tya", OpCode::AddressingMode::Implied, tya}},
};

std::string OpCode::to_string() const {
  using namespace std;

  stringstream oss;

  oss << "  " << name;
  oss << setfill(' ') << setw(7);
  switch (addressing_mode) {
    case OpCode::Accumulator: oss << "Acc";
      break;
    case OpCode::Absolute: oss << "Abs";
      break;
    case OpCode::AbsoluteIndexedX: oss << "Abs,X";
      break;
    case OpCode::AbsoluteIndexedY: oss << "Abs,Y";
      break;
    case OpCode::Immediate: oss << "Imm";
      break;
    case OpCode::Implied: oss << "";
      break;
    case OpCode::Indirect: oss << "Ind";
      break;
    case OpCode::IndirectIndexedX: oss << "Ind,X";
      break;
    case OpCode::IndirectIndexedY: oss << "Ind,Y";
      break;
    case OpCode::Relative: oss << "Rel";
      break;
    case OpCode::ZeroPage: oss << "Zpg";
      break;
    case OpCode::ZeroPageIndexedX: oss << "Zpg,X";
      break;
    case OpCode::ZeroPageIndexedY: oss << "Zpg,Y";
      break;
    default: oss << "ERR";
      break;
  }
  oss << "    | ";
  return oss.str();

}