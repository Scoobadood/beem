#include "acorn_block.h"
#include <string>
#include <spdlog/spdlog.h>

#include "crc-16-acorn.h"

void check_sync_byte(const std::vector<uint8_t> &data, uint32_t &idx) {
  if (data.at(idx) != 0x2A) {
    throw std::runtime_error("Invalid sync byte in data");
  }
  idx++;
}

std::string read_file_name(const std::vector<uint8_t> &data, uint32_t &idx) {
  auto file_name_idx = 0;
  char file_name[11]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  do {
    file_name[file_name_idx++] = data[idx++];
  } while (file_name_idx != 10 && data[idx] != 0);

  if (data[idx] != 0) {
    throw std::runtime_error(fmt::format("file name not terminated in block of file {}", file_name));
  }
  idx++;
  return std::string(file_name);
}

uint32_t read_uint32(const std::vector<uint8_t> &data, uint32_t &idx) {
  idx += 4;
  return data[idx - 4] | (data[idx - 3] << 8) | (data[idx - 2] << 16) | (data[idx - 1] << 24);
}

uint16_t read_uint16(const std::vector<uint8_t> &data, uint32_t &idx) {
  idx += 2;
  return data[idx - 2] | (data[idx - 1] << 8);
}

// Read big endian - used for CRC comparisons
uint16_t read_uint16_be(const std::vector<uint8_t> &data, uint32_t &idx) {
  idx += 2;
  return data[idx - 1] | (data[idx - 2] << 8);
}

AcornBlock::AcornBlock(const std::vector<uint8_t> &data, uint32_t offset, uint32_t &block_size) {
  uint32_t idx = offset;
  check_sync_byte(data, idx);
  file_name_ = read_file_name(data, idx);
  load_address_ = (read_uint32(data, idx) & 0xffff);
  execution_address_ = (read_uint32(data, idx) & 0xffff);
  block_number_ = read_uint16(data, idx);

  auto data_length = read_uint16(data, idx);
  block_flags_ = data.at(idx++);
  next_file_ = read_uint32(data, idx);
  auto alleged_header_crc = read_uint16_be(data, idx);
  auto header_length = 18 + file_name_.length();
  // Exclude the sync byte from the crc
  auto computed_header_crc = crc(data, offset + 1, header_length);
  if (alleged_header_crc != computed_header_crc) {
    throw std::runtime_error(fmt::format("Header CRC check failed loading block {} of file {}",
                                         block_number_,
                                         file_name_));
  }

  data_.insert(data_.end(), data.data() + idx, data.data() + idx + data_length);
  auto computed_data_crc = crc(data, idx, data_length);
  idx += data_length;
  auto alleged_data_crc = read_uint16_be(data, idx);
  if (alleged_data_crc != computed_data_crc) {
    throw std::runtime_error(fmt::format("Data CRC check failed loading block {} of file {}",
                                         block_number_,
                                         file_name_));
  }
  block_size = idx - offset;
}
