#ifndef LIBS_UEF_STREAM_UTILS_H_
#define LIBS_UEF_STREAM_UTILS_H_

#include <cstdint>
#include <iostream>

uint16_t read_uint_16(std::unique_ptr<std::istream> &uef_stream);
uint32_t read_uint_32(std::unique_ptr<std::istream> &uef_stream);
float    read_iee_754_float(std::unique_ptr<std::istream> &uef_stream);

#endif // LIBS_UEF_STREAM_UTILS_H_
