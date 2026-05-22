#ifndef LIBS_UEF_H_
#define LIBS_UEF_H_

#include <cstdint>
#include <vector>
#include <map>
#include <spdlog/fmt/fmt.h>

#include "chunk.h"

class UefData {
 public:
  explicit UefData(std::unique_ptr<std::istream> &uef_stream);
  static UefData FromStream(std::unique_ptr<std::istream> &uef_stream);
  static UefData FromFile(const std::string &file_name);
  inline uint8_t MajorVersion() const { return major_version_; }
  inline uint8_t MinorVersion() const { return minor_version_; }
  inline std::string StringVersion() const { return fmt::format("{}.{}", (int)major_version_, (int)minor_version_); }
  const std::vector<std::shared_ptr<Chunk>> &Chunks() const;

 private:
  void ReadChunks(std::unique_ptr<std::istream> &uef_stream);

  uint8_t major_version_;
  uint8_t minor_version_;
  std::vector<std::shared_ptr<Chunk>> chunks_;

};

struct TapeFile {
  std::string name;
  std::uint16_t load_addr;
  std::uint16_t exec_addr;
  std::vector<uint8_t> data;
};

std::map<std::string, std::shared_ptr<TapeFile>> load_tape_data_from_uef(const std::string &file_name);
std::map<std::string, std::shared_ptr<TapeFile>> load_tape_data_from_uef(const UefData &uef);

#endif // LIBS_UEF_H_
