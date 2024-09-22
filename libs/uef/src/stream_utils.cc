#include <cmath>
#include "stream_utils.h"

uint16_t read_uint_16(std::unique_ptr<std::istream> &uef_stream){
  uint8_t data[2];
  uef_stream->read(reinterpret_cast<char *>(data), 2);
  return data[0] | (data[1] << 8);
}

uint32_t read_uint_32(std::unique_ptr<std::istream> &uef_stream) {
  uint8_t len[4];
  uef_stream->read((char *) len, 4);
  return len[0] | (len[1] << 8) | (len[2] << 16) | (len[3] << 24);
}

float read_iee_754_float(std::unique_ptr<std::istream> &uef_stream){
  uint8_t data[4];
  uef_stream->read((char *) data, 4);

  int32_t mantissa = data[0] | (data[1] << 8) | ((data[2]&0x7f)|0x80) << 16;
  float result = (float)mantissa;
  result = (float)ldexp(result, -23);

  int exponent;
  exponent = ((data[2]&0x80) >> 7) | (data[3]&0x7f) << 1;
  exponent -= 127;
  result = (float)ldexp(result, exponent);

  if(data[3]&0x80)
    result = -result;

  return result;
}
