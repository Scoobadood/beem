#ifndef LIBS_UEF_ACORN_BLOCK_H_
#define LIBS_UEF_ACORN_BLOCK_H_

#include <cstdint>
#include <vector>
#include <string>

class AcornBlock {
 public:
  AcornBlock(const std::vector<uint8_t> &data, uint32_t offset, uint32_t & block_size);
  const std::string & FileName() const { return file_name_;}
  const std::vector<uint8_t> & Data() const { return data_;}
  uint16_t LoadAddress() const {return load_address_;}
  uint16_t ExecutionAddress() const {return execution_address_;}
  uint16_t BlockNumber() const { return block_number_;}
  uint8_t Flags() const { return block_flags_;}

 private:
  std::string file_name_;
  // High bytes are guaranteed &ffff
  uint16_t load_address_;
  // High bytes are guaranteed &ffff
  uint16_t execution_address_;
  uint16_t block_number_;
  uint8_t block_flags_;
  uint32_t next_file_;
  std::vector<uint8_t> data_;
};
#endif // LIBS_UEF_ACORN_BLOCK_H_
