//
// Created by Dave Durbin on 1/12/2022.
//

#include "opcodes.h"
#include "addressing.h"

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

#define DEBUG_ADC
#define DEBUG_SBC

const uint32_t STACK_BASE = 0x100;
void push_stack(Cpu &cpu, std::vector<uint8_t> &memory, uint8_t arg) {
  memory.at(STACK_BASE + cpu.stack_pointer_) = (arg & 0xff);
  cpu.stack_pointer_ = (cpu.stack_pointer_ - 1) & 0xff;
}

uint8_t pop_stack(Cpu &cpu, std::vector<uint8_t> &memory) {
  cpu.stack_pointer_ = (cpu.stack_pointer_ + 1) & 0xff;
  return memory.at(STACK_BASE + cpu.stack_pointer_) & 0xff;
}

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

void adc_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void adc_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}
void adc_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}
void adc_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}
void adc_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}
void adc_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}
void adc_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}
void adc_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  adc(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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

void and_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void and_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void and_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void and_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void and_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void and_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void and_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}

void and_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  anda(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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

  arg <<= 1;
  cpu.status_.set(SR_NEG, (arg & 0x80));
  cpu.status_.set(SR_ZER, (arg == 0));
  return (arg & 0xff);
}

void asl_a(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.accumulator_ = asl(cpu, cpu.accumulator_);
  clk += 2;
}

void asl_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPage(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.at(addr) = arg;
}

void asl_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedX(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.at(addr) = arg;
}

void asl_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = Absolute(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.at(addr) = arg;
}

void asl_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedX(cpu, memory, addr, page_wrap);
  arg = asl(cpu, arg);
  memory.at(addr) = arg;
}

void do_branch(Cpu &cpu, std::vector<uint8_t> &memory, int8_t branch, uint64_t &clk) {
  // Branch taken, inc clk.
  clk++;

  auto old_addr_hi = (cpu.pc_ >> 8) & 0xff;
  cpu.pc_ += branch;
  auto new_addr_hi = (cpu.pc_ >> 8) & 0xff;
  if (old_addr_hi != new_addr_hi) {
    clk++;
  }
}

//Branch on Carry Clear
//
//branch on C = 0
//N	Z	C	I	D	V
//-	-	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    relative	BCC oper	90	2	2**
void bcc(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
  if (cpu.carry_clear()) {
    do_branch(cpu, memory, branch, clk);
  }
}

//Branch on Carry Set
//
//branch on C = 1
//N	Z	C	I	D	V
//-	-	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    relative	BCS oper	B0	2	2**
void bcs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
  if (cpu.carry()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void beq(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
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

void bit_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  bit(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void bit_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  bit(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void bmi(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
  if (cpu.minus()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void bne(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
  if (cpu.not_zero()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void bpl(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
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
void brk(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  // PC currently points to the reson byte after BRK
  auto pc = cpu.pc_ + 1;
  push_stack(cpu, memory, pc >> 8);
  push_stack(cpu, memory, pc & 0xff);

  auto status = (cpu.status_.to_ulong());
  status |= BRK_FLAG;
  status |= RES_FLAG;
  push_stack(cpu, memory, status);

  cpu.set_interrupt();

  cpu.pc_ = (memory.at(0xfffe) + (memory.at(0xffff) * 256)) & 0xffff;
  clk += 7;
}

void bvc(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
  if (!cpu.is_overflow()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void bvs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  clk += 2;
  auto branch = reinterpret_cast<int8_t &>(memory.at(cpu.pc_++));
  if (cpu.is_overflow()) {
    do_branch(cpu, memory, branch, clk);
  }
}

void clc(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.clear_carry();
}

void cld(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.clear_decimal();
}

void cli(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.clear_interrupt();
}

void clv(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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

void cmp_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void cmp_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void cmp_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void cmp_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void cmp_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void cmp_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void cmp_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}

void cmp_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cmp(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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

void cpx_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpx(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void cpx_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpx(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void cpx_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpx(cpu, Absolute(cpu, memory, addr, page_wrap));
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

void cpy_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpy(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void cpy_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpy(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void cpy_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  cpy(cpu, Absolute(cpu, memory, addr, page_wrap));
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

void dec_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
  clk += 5;
}

void dec_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
  clk += 6;
}

void dec_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, Absolute(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
  clk += 6;
}

void dec_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = dec(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
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
void dex(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void dey(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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

void eor_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void eor_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void eor_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void eor_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void eor_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void eor_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void eor_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}

void eor_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  eor(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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

void inc_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
  clk += 5;
}

void inc_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
  clk += 6;
}

void inc_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, Absolute(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
  clk += 6;
}

void inc_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto ans = inc(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  memory.at(addr) = ans;
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
void inx(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void iny(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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

void jmp_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void jmp_ind(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void jsr(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  // PC currently points at first byte of arg
  auto ret_addr = cpu.pc_ + 1;

  push_stack(cpu, memory, ret_addr >> 8);
  push_stack(cpu, memory, ret_addr & 0xff);
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

void lda_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void lda_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void lda_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void lda_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void lda_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void lda_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void lda_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}

void lda_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  lda(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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

void ldx_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void ldx_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void ldx_zpg_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, ZeroPageIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldx_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldx_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldx(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
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

void ldy_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void ldy_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void ldy_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldy_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ldy_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ldy(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
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

void lsr_a(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.accumulator_ = lsr(cpu, cpu.accumulator_);
  clk += 2;
}

void lsr_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPage(cpu, memory, addr, page_wrap);
  memory.at(addr) = lsr(cpu, arg);
  clk += 5;
}

void lsr_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedX(cpu, memory, addr, page_wrap);
  memory.at(addr) = lsr(cpu, arg);;
  clk += 6;
}

void lsr_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = Absolute(cpu, memory, addr, page_wrap);
  memory.at(addr) = lsr(cpu, arg);
  clk += 6;
}

void lsr_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedX(cpu, memory, addr, page_wrap);
  memory.at(addr) = lsr(cpu, arg);
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
void nop(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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

void ora_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void ora_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void ora_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ora_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void ora_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void ora_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void ora_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}

void ora_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  or_a(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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
void pha(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  push_stack(cpu, memory, cpu.accumulator_);
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
void php(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  auto arg = cpu.status_.to_ulong();
  arg |= BRK_FLAG;
  arg |= RES_FLAG;
  push_stack(cpu, memory, arg);

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
void pla(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.accumulator_ = pop_stack(cpu, memory);
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
void plp(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.status_ = pop_stack(cpu, memory);
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

void rol_a(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = cpu.accumulator_;
  arg = rol(cpu, arg);
  cpu.accumulator_ = arg;
  clk += 2;
}

void rol_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPage(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.at(addr) = arg;
  clk += 5;
}

void rol_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedX(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.at(addr) = arg;
  clk += 6;
}

void rol_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = Absolute(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.at(addr) = arg;
  clk += 6;
}

void rol_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedX(cpu, memory, addr, page_wrap);
  arg = rol(cpu, arg);
  memory.at(addr) = arg;
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

void ror_a(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = cpu.accumulator_;
  arg = ror(cpu, arg);
  cpu.accumulator_ = arg;
  clk += 2;
}

void ror_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPage(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.at(addr) = arg;
  clk += 5;
}

void ror_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = ZeroPageIndexedX(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.at(addr) = arg;
  clk += 6;
}

void ror_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = Absolute(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.at(addr) = arg;
  clk += 6;
}

void ror_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  auto arg = AbsoluteIndexedX(cpu, memory, addr, page_wrap);
  arg = ror(cpu, arg);
  memory.at(addr) = arg;
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

void rti(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.status_ = pop_stack(cpu, memory);

  auto pcl = pop_stack(cpu, memory);
  auto pch = pop_stack(cpu, memory);
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
void rts(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  auto pcl = pop_stack(cpu, memory);
  auto pch = pop_stack(cpu, memory);
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
  if( a < 0 ) a -= 0x60;

  cpu.status_.set(SR_CRY, cpu.accumulator_ >= (arg + borrow) );

  //  3e. The accumulator result is the lower 8 bits of A
  cpu.accumulator_ = a & 0xff;

  /* The flags are set just like in Binary mode. */
  int32_t bin_val = cpu.accumulator_ - arg - (cpu.carry_clear() ? 1 : 0);
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

void sbc_imm(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, Immediate(cpu, memory, addr, page_wrap));
  clk += 2;
}

void sbc_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, ZeroPage(cpu, memory, addr, page_wrap));
  clk += 3;
}

void sbc_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, ZeroPageIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
}

void sbc_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, Absolute(cpu, memory, addr, page_wrap));
  clk += 4;
}

void sbc_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, AbsoluteIndexedX(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void sbc_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, AbsoluteIndexedY(cpu, memory, addr, page_wrap));
  clk += 4;
  if (page_wrap) clk++;
}

void sbc_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, IndexedIndirect(cpu, memory, addr, page_wrap));
  clk += 6;
}

void sbc_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  sbc(cpu, IndirectIndexed(cpu, memory, addr, page_wrap));
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
void sec(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void sed(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void sei(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void sta_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ZeroPage(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
  clk += 3;
}

void sta_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ZeroPageIndexedX(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
  clk += 4;
}

void sta_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  Absolute(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
  clk += 4;
}

void sta_abs_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  AbsoluteIndexedX(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
  clk += 5;
}

void sta_abs_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  AbsoluteIndexedY(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
  clk += 5;
}

void sta_ind_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  IndexedIndirect(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
  clk += 6;
}

void sta_ind_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  IndirectIndexed(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.accumulator_;
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
void stx_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ZeroPage(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.x_reg_;
  clk += 3;
}
void stx_zpg_y(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ZeroPageIndexedY(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.x_reg_;
  clk += 4;
}
void stx_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  Absolute(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.x_reg_;
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
void sty_zpg(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ZeroPage(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.y_reg_;
  clk += 3;
}
void sty_zpg_x(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  ZeroPageIndexedX(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.y_reg_;
  clk += 4;
}
void sty_abs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  bool page_wrap;
  uint32_t addr;
  Absolute(cpu, memory, addr, page_wrap);
  memory.at(addr) = cpu.y_reg_;
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
void tax(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void tay(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void tsx(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.x_reg_ = (cpu.stack_pointer_ & 0xff);
  cpu.status_.set(SR_NEG, cpu.x_reg_ & 0x80);
  cpu.status_.set(SR_ZER, cpu.x_reg_ == 0);
  clk += 2;
}

//TXA
//    Transfer Index X to Accumulator
//
//X -> A
//    N	Z	C	I	D	V
//+	+	-	-	-	-
//addressing	assembler	opc	bytes	cycles
//    implied	TXA	8A	1	2
void txa(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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

void txs(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
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
void tya(Cpu &cpu, std::vector<uint8_t> &memory, uint64_t &clk) {
  cpu.accumulator_ = cpu.y_reg_;
  if (cpu.accumulator_ & 0x80) cpu.set_neg(); else cpu.clear_neg();
  if (cpu.accumulator_ == 0) cpu.set_zero(); else cpu.clear_zero();
  clk += 2;
}

const std::map<uint8_t, OpCode> codes = {
    {0x69, {2, 2, false, "adc", OpCode::AddressingMode::Immediate, adc_imm}},
    {0x65, {2, 3, false, "adc", OpCode::AddressingMode::ZeroPage, adc_zpg}},
    {0x75, {2, 4, false, "adc", OpCode::AddressingMode::ZeroPageIndexedX, adc_zpg_x}},
    {0x6d, {3, 4, false, "adc", OpCode::AddressingMode::Absolute, adc_abs}},
    {0x7d, {3, 4, true, "adc", OpCode::AddressingMode::AbsoluteIndexedX, adc_abs_x}},
    {0x79, {3, 4, true, "adc", OpCode::AddressingMode::AbsoluteIndexedY, adc_abs_y}},
    {0x61, {2, 6, false, "adc", OpCode::AddressingMode::IndirectIndexedX, adc_ind_x}},
    {0x71, {2, 5, false, "adc", OpCode::AddressingMode::IndirectIndexedY, adc_ind_y}},
    {0x29, {2, 2, false, "and", OpCode::AddressingMode::Immediate, and_imm}},
    {0x25, {2, 3, false, "and", OpCode::AddressingMode::ZeroPage, and_zpg}},
    {0x35, {2, 4, false, "and", OpCode::AddressingMode::ZeroPageIndexedX, and_zpg_x}},
    {0x2d, {3, 4, false, "and", OpCode::AddressingMode::Absolute, and_abs}},
    {0x3d, {3, 4, true, "and", OpCode::AddressingMode::AbsoluteIndexedX, and_abs_x}},
    {0x39, {3, 4, true, "and", OpCode::AddressingMode::AbsoluteIndexedY, and_abs_y}},
    {0x21, {2, 6, false, "and", OpCode::AddressingMode::IndirectIndexedX, and_ind_x}},
    {0x31, {2, 5, false, "and", OpCode::AddressingMode::IndirectIndexedY, and_ind_y}},
    {0x0a, {1, 2, false, "asl", OpCode::AddressingMode::Accumulator, asl_a}},
    {0x06, {2, 5, false, "asl", OpCode::AddressingMode::ZeroPage, asl_zpg}},
    {0x16, {2, 6, false, "asl", OpCode::AddressingMode::ZeroPageIndexedX, asl_zpg_x}},
    {0x0e, {3, 6, false, "asl", OpCode::AddressingMode::Absolute, asl_abs}},
    {0x1e, {3, 7, false, "asl", OpCode::AddressingMode::AbsoluteIndexedX, asl_abs_x}},
    {0x90, {2, 2, true, "bcc", OpCode::AddressingMode::Relative, bcc}},
    {0xb0, {2, 2, true, "bcs", OpCode::AddressingMode::Relative, bcs}},
    {0xf0, {2, 2, true, "beq", OpCode::AddressingMode::Relative, beq}},
    {0x30, {2, 2, true, "bmi", OpCode::AddressingMode::Relative, bmi}},
    {0xd0, {2, 2, true, "bne", OpCode::AddressingMode::Relative, bne}},
    {0x10, {2, 2, true, "bpl", OpCode::AddressingMode::Relative, bpl}},
    {0x50, {2, 2, true, "bvc", OpCode::AddressingMode::Relative, bvc}},
    {0x70, {2, 2, true, "bvs", OpCode::AddressingMode::Relative, bvs}},
    {0x00, {1, 7, false, "brk", OpCode::AddressingMode::Implied, brk}},
    {0x18, {1, 2, false, "clc", OpCode::AddressingMode::Implied, clc}},
    {0xd8, {1, 2, false, "cld", OpCode::AddressingMode::Implied, cld}},
    {0x58, {1, 2, false, "cli", OpCode::AddressingMode::Implied, cli}},
    {0xb8, {1, 2, false, "clv", OpCode::AddressingMode::Implied, clv}},
    {0x24, {2, 3, false, "bit", OpCode::AddressingMode::ZeroPage, bit_zpg}},
    {0x2c, {3, 4, false, "bit", OpCode::AddressingMode::Absolute, bit_abs}},
    {0xc9, {2, 2, false, "cmp", OpCode::AddressingMode::Immediate, cmp_imm}},
    {0xc5, {2, 3, false, "cmp", OpCode::AddressingMode::ZeroPage, cmp_zpg}},
    {0xd5, {2, 4, false, "cmp", OpCode::AddressingMode::ZeroPageIndexedX, cmp_zpg_x}},
    {0xcd, {3, 4, false, "cmp", OpCode::AddressingMode::Absolute, cmp_abs}},
    {0xdd, {3, 4, true, "cmp", OpCode::AddressingMode::AbsoluteIndexedX, cmp_abs_x}},
    {0xd9, {3, 4, true, "cmp", OpCode::AddressingMode::AbsoluteIndexedY, cmp_abs_y}},
    {0xc1, {2, 6, false, "cmp", OpCode::AddressingMode::IndirectIndexedX, cmp_ind_x}},
    {0xd1, {2, 5, false, "cmp", OpCode::AddressingMode::IndirectIndexedY, cmp_ind_y}},
    {0xe0, {2, 2, false, "cpx", OpCode::AddressingMode::Immediate, cpx_imm}},
    {0xe4, {2, 3, false, "cpx", OpCode::AddressingMode::ZeroPage, cpx_zpg}},
    {0xec, {3, 4, false, "cpx", OpCode::AddressingMode::Absolute, cpx_abs}},
    {0xc0, {2, 2, false, "cpy", OpCode::AddressingMode::Immediate, cpy_imm}},
    {0xc4, {2, 3, false, "cpy", OpCode::AddressingMode::ZeroPage, cpy_zpg}},
    {0xcc, {3, 4, false, "cpy", OpCode::AddressingMode::Absolute, cpy_abs}},
    {0xc6, {2, 5, false, "dec", OpCode::AddressingMode::ZeroPage, dec_zpg}},
    {0xd6, {2, 6, false, "dec", OpCode::AddressingMode::ZeroPageIndexedX, dec_zpg_x}},
    {0xce, {3, 6, false, "dec", OpCode::AddressingMode::Absolute, dec_abs}},
    {0xde, {3, 7, false, "dec", OpCode::AddressingMode::AbsoluteIndexedX, dec_abs_x}},
    {0xca, {1, 2, false, "dex", OpCode::AddressingMode::Implied, dex}},
    {0x88, {1, 2, false, "dey", OpCode::AddressingMode::Implied, dey}},

    {0x49, {2, 2, false, "eor", OpCode::AddressingMode::Immediate, eor_imm}},
    {0x45, {2, 3, false, "eor", OpCode::AddressingMode::ZeroPage, eor_zpg}},
    {0x55, {2, 4, false, "eor", OpCode::AddressingMode::ZeroPageIndexedX, eor_zpg_x}},
    {0x4d, {3, 4, false, "eor", OpCode::AddressingMode::Absolute, eor_abs}},
    {0x5d, {3, 4, true, "eor", OpCode::AddressingMode::AbsoluteIndexedX, eor_abs_x}},
    {0x59, {3, 4, true, "eor", OpCode::AddressingMode::AbsoluteIndexedY, eor_abs_y}},
    {0x41, {2, 6, false, "eor", OpCode::AddressingMode::IndirectIndexedX, eor_ind_x}},
    {0x51, {2, 5, true, "eor", OpCode::AddressingMode::IndirectIndexedY, eor_ind_y}},

    {0xe6, {2, 5, false, "inc", OpCode::AddressingMode::ZeroPage, inc_zpg}},
    {0xf6, {2, 6, false, "inc", OpCode::AddressingMode::ZeroPageIndexedX, inc_zpg_x}},
    {0xee, {3, 6, false, "inc", OpCode::AddressingMode::Absolute, inc_abs}},
    {0xfe, {3, 7, false, "inc", OpCode::AddressingMode::AbsoluteIndexedX, inc_abs_x}},
    {0xe8, {1, 2, false, "inx", OpCode::AddressingMode::Implied, inx}},
    {0xc8, {1, 2, false, "iny", OpCode::AddressingMode::Implied, iny}},

    {0x4c, {3, 3, false, "jmp", OpCode::AddressingMode::Absolute, jmp_abs}},
    {0x6c, {3, 5, false, "jmp", OpCode::AddressingMode::Indirect, jmp_ind}},
    {0x20, {3, 6, false, "jsr", OpCode::AddressingMode::Absolute, jsr}},

    {0xa5, {2, 2, false, "lda", OpCode::AddressingMode::ZeroPage, lda_zpg}},
    {0xa9, {2, 3, false, "lda", OpCode::AddressingMode::Immediate, lda_imm}},
    {0xad, {2, 4, false, "lda", OpCode::AddressingMode::Absolute, lda_abs}},
    {0xb5, {3, 4, false, "lda", OpCode::AddressingMode::ZeroPageIndexedX, lda_zpg_x}},
    {0xb9, {3, 4, true, "lda", OpCode::AddressingMode::AbsoluteIndexedY, lda_abs_y}},
    {0xbd, {3, 4, true, "lda", OpCode::AddressingMode::AbsoluteIndexedX, lda_abs_x}},
    {0xa1, {2, 6, false, "lda", OpCode::AddressingMode::IndirectIndexedX, lda_ind_x}},
    {0xb1, {2, 5, true, "lda", OpCode::AddressingMode::IndirectIndexedY, lda_ind_y}},

    {0xa2, {2, 2, false, "ldx", OpCode::AddressingMode::Immediate, ldx_imm}},
    {0xa6, {2, 3, false, "ldx", OpCode::AddressingMode::ZeroPage, ldx_zpg}},
    {0xb6, {2, 4, false, "ldx", OpCode::AddressingMode::ZeroPageIndexedY, ldx_zpg_y}},
    {0xae, {3, 4, false, "ldx", OpCode::AddressingMode::Absolute, ldx_abs}},
    {0xbe, {3, 4, true, "ldx", OpCode::AddressingMode::AbsoluteIndexedY, ldx_abs_y}},

    {0xa0, {2, 2, false, "ldy", OpCode::AddressingMode::Immediate, ldy_imm}},
    {0xa4, {2, 3, false, "ldy", OpCode::AddressingMode::ZeroPage, ldy_zpg}},
    {0xb4, {2, 4, false, "ldy", OpCode::AddressingMode::ZeroPageIndexedX, ldy_zpg_x}},
    {0xac, {3, 4, false, "ldy", OpCode::AddressingMode::Absolute, ldy_abs}},
    {0xbc, {3, 4, true, "ldy", OpCode::AddressingMode::AbsoluteIndexedX, ldy_abs_x}},

    {0x4a, {1, 2, false, "lsr", OpCode::AddressingMode::Accumulator, lsr_a}},
    {0x46, {2, 5, false, "lsr", OpCode::AddressingMode::ZeroPage, lsr_zpg}},
    {0x56, {2, 6, false, "lsr", OpCode::AddressingMode::ZeroPageIndexedX, lsr_zpg_x}},
    {0x4e, {3, 6, false, "lsr", OpCode::AddressingMode::Absolute, lsr_abs}},
    {0x5e, {3, 7, false, "lsr", OpCode::AddressingMode::AbsoluteIndexedX, lsr_abs_x}},

    {0xea, {1, 2, false, "nop", OpCode::AddressingMode::Implied, nop}},

    {0x09, {2, 2, false, "ora", OpCode::AddressingMode::Immediate, ora_imm}},
    {0x05, {2, 3, false, "ora", OpCode::AddressingMode::ZeroPage, ora_zpg}},
    {0x15, {2, 4, false, "ora", OpCode::AddressingMode::ZeroPageIndexedX, ora_zpg_x}},
    {0x0d, {3, 4, false, "ora", OpCode::AddressingMode::Absolute, ora_abs}},
    {0x1d, {3, 4, true, "ora", OpCode::AddressingMode::AbsoluteIndexedX, ora_abs_x}},
    {0x19, {3, 4, true, "ora", OpCode::AddressingMode::AbsoluteIndexedY, ora_abs_y}},
    {0x01, {2, 6, false, "ora", OpCode::AddressingMode::IndirectIndexedX, ora_ind_x}},
    {0x11, {2, 5, true, "ora", OpCode::AddressingMode::IndirectIndexedY, ora_ind_y}},

    {0x48, {1, 3, false, "pha", OpCode::AddressingMode::Implied, pha}},
    {0x08, {1, 3, false, "php", OpCode::AddressingMode::Implied, php}},
    {0x68, {1, 4, false, "pla", OpCode::AddressingMode::Implied, pla}},
    {0x28, {1, 4, false, "plp", OpCode::AddressingMode::Implied, plp}},

    {0x2a, {1, 2, false, "rol", OpCode::AddressingMode::Accumulator, rol_a}},
    {0x26, {2, 5, false, "rol", OpCode::AddressingMode::ZeroPage, rol_zpg}},
    {0x36, {2, 6, false, "rol", OpCode::AddressingMode::ZeroPageIndexedX, rol_zpg_x}},
    {0x2e, {3, 6, false, "rol", OpCode::AddressingMode::Absolute, rol_abs}},
    {0x3e, {3, 7, false, "rol", OpCode::AddressingMode::AbsoluteIndexedX, rol_abs_x}},

    {0x6a, {1, 2, false, "ror", OpCode::AddressingMode::Accumulator, ror_a}},
    {0x66, {2, 5, false, "ror", OpCode::AddressingMode::ZeroPage, ror_zpg}},
    {0x76, {2, 6, false, "ror", OpCode::AddressingMode::ZeroPageIndexedX, ror_zpg_x}},
    {0x6e, {3, 6, false, "ror", OpCode::AddressingMode::Absolute, ror_abs}},
    {0x7e, {3, 7, false, "ror", OpCode::AddressingMode::AbsoluteIndexedX, ror_abs_x}},

    {0x40, {1, 6, false, "rti", OpCode::AddressingMode::Implied, rti}},
    {0x60, {1, 6, false, "rts", OpCode::AddressingMode::Implied, rts}},

    {0xe9, {2, 2, false, "sbc", OpCode::AddressingMode::Immediate, sbc_imm}},
    {0xe5, {2, 3, false, "sbc", OpCode::AddressingMode::ZeroPage, sbc_zpg}},
    {0xf5, {2, 4, false, "sbc", OpCode::AddressingMode::ZeroPageIndexedX, sbc_zpg_x}},
    {0xed, {3, 4, false, "sbc", OpCode::AddressingMode::Absolute, sbc_abs}},
    {0xfd, {3, 4, true, "sbc", OpCode::AddressingMode::AbsoluteIndexedX, sbc_abs_x}},
    {0xf9, {3, 4, true, "sbc", OpCode::AddressingMode::AbsoluteIndexedY, sbc_abs_y}},
    {0xe1, {2, 6, false, "sbc", OpCode::AddressingMode::IndirectIndexedX, sbc_ind_x}},
    {0xf1, {2, 5, true, "sbc", OpCode::AddressingMode::IndirectIndexedY, sbc_ind_y}},

    {0x38, {1, 2, false, "sec", OpCode::AddressingMode::Implied, sec}},
    {0xf8, {1, 2, false, "sed", OpCode::AddressingMode::Implied, sed}},
    {0x78, {1, 2, false, "sei", OpCode::AddressingMode::Implied, sei}},

    {0x85, {2, 3, false, "sta", OpCode::AddressingMode::ZeroPage, sta_zpg}},
    {0x95, {2, 4, false, "sta", OpCode::AddressingMode::ZeroPageIndexedX, sta_zpg_x}},
    {0x8d, {3, 4, false, "sta", OpCode::AddressingMode::Absolute, sta_abs}},
    {0x9d, {3, 5, false, "sta", OpCode::AddressingMode::AbsoluteIndexedX, sta_abs_x}},
    {0x99, {3, 5, false, "sta", OpCode::AddressingMode::AbsoluteIndexedY, sta_abs_y}},
    {0x81, {2, 6, false, "sta", OpCode::AddressingMode::IndirectIndexedX, sta_ind_x}},
    {0x91, {2, 6, false, "sta", OpCode::AddressingMode::IndirectIndexedY, sta_ind_y}},

    {0x86, {2, 3, false, "stx", OpCode::AddressingMode::ZeroPage, stx_zpg}},
    {0x96, {2, 4, false, "stx", OpCode::AddressingMode::ZeroPageIndexedY, stx_zpg_y}},
    {0x8e, {3, 4, false, "stx", OpCode::AddressingMode::Absolute, stx_abs}},

    {0x84, {2, 3, false, "sty", OpCode::AddressingMode::ZeroPage, sty_zpg}},
    {0x94, {2, 4, false, "sty", OpCode::AddressingMode::ZeroPageIndexedX, sty_zpg_x}},
    {0x8c, {3, 4, false, "sty", OpCode::AddressingMode::Absolute, sty_abs}},

    {0xaa, {1, 2, false, "tax", OpCode::AddressingMode::Implied, tax}},
    {0xa8, {1, 2, false, "tay", OpCode::AddressingMode::Implied, tay}},
    {0xba, {1, 2, false, "tsx", OpCode::AddressingMode::Implied, tsx}},
    {0x8a, {1, 2, false, "txa", OpCode::AddressingMode::Implied, txa}},
    {0x9a, {1, 2, false, "txs", OpCode::AddressingMode::Implied, txs}},
    {0x98, {1, 2, false, "tya", OpCode::AddressingMode::Implied, tya}},
};



