//
// Created by Dave Durbin on 1/12/2022.
//

#ifndef CPU_ADDRESSING_H_
#define CPU_ADDRESSING_H_

#include "m6502.h"
#include "memory.h"

#include <vector>


using AddressingFunction = std::function<uint32_t(M6502 &cpu,
                                                  Memory &memory,
                                                  uint32_t &read_addr,
                                                  bool &page_wrap)>;
using AddressComputeFunction = std::function<uint32_t(M6502 &cpu,
                                                      Memory &memory,
                                                      bool &page_wrap)>;

extern AddressingFunction ImmediateData;
extern AddressingFunction ZeroPageData;
extern AddressingFunction ZeroPageIndexedXData;
extern AddressingFunction ZeroPageIndexedYData;
extern AddressingFunction AbsoluteData;
extern AddressingFunction AbsoluteIndexedXData;
extern AddressingFunction AbsoluteIndexedYData;
extern AddressingFunction IndirectAbsoluteData;
extern AddressingFunction IndexedIndirectData;
extern AddressingFunction IndirectIndexedData;

extern AddressComputeFunction AbsoluteAddress;
extern AddressComputeFunction AbsoluteIndexedYAddress;
extern AddressComputeFunction AbsoluteIndexedXAddress;
extern AddressComputeFunction ZeroPageAddress;
extern AddressComputeFunction ZeroPageIndexedXAddress;
extern AddressComputeFunction ZeroPageIndexedYAddress;
extern AddressComputeFunction IndexedIndirectAddress;
extern AddressComputeFunction IndirectIndexedAddress;

#endif //CPU_ADDRESSING_H_
