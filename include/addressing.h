//
// Created by Dave Durbin on 1/12/2022.
//

#ifndef CPU_ADDRESSING_H_
#define CPU_ADDRESSING_H_

#include "cpu.h"
#include "memory.h"

#include <vector>


using AddressingFunction = std::function<uint32_t(Cpu &cpu,
                                                  Memory &memory,
                                                  uint32_t &read_addr,
                                                  bool &page_wrap)>;
extern AddressingFunction Immediate;
extern AddressingFunction ZeroPage;
extern AddressingFunction ZeroPageIndexedX;
extern AddressingFunction ZeroPageIndexedY;
extern AddressingFunction Absolute;
extern AddressingFunction AbsoluteIndexedX;
extern AddressingFunction AbsoluteIndexedY;
extern AddressingFunction IndirectAbsolute;
extern AddressingFunction IndexedIndirect;
extern AddressingFunction IndirectIndexed;

#endif //CPU_ADDRESSING_H_
