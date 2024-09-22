#ifndef LIBS_UEF_CRC_16_ACORN_H_
#define LIBS_UEF_CRC_16_ACORN_H_

#include <vector>

uint16_t crc(const std::vector<uint8_t>& data, uint32_t offset, uint32_t length);

#endif // LIBS_UEF_CRC_16_ACORN_H_
