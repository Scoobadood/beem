#include <UEF/uef.h>
#include "tape_data_chunk.h"
#include "acorn_block.h"

#include <spdlog/spdlog.h>

std::vector<std::shared_ptr<AcornBlock>> parse_blocks(const std::vector<uint8_t> &data) {
  std::vector<std::shared_ptr<AcornBlock>> blocks;
  uint32_t idx = 0;
  uint32_t blk_size = 0;
  while (idx < data.size()) {
    auto blk_data_size = data.size() - idx;
    if (blk_data_size < 19) {
      spdlog::error("A data block reported as tape data is too short, {} bytes", blk_data_size);
      return {};
    }
    blocks.push_back(std::make_shared<AcornBlock>(data, idx, blk_size));
    idx += blk_size;
  }
  return blocks;
}

std::map<std::string, std::shared_ptr<TapeFile>>
load_tape_data_from_uef(const UefData &uef) {
  std::map<std::string, std::shared_ptr<TapeFile>> files;
  for (const auto &chunk : uef.Chunks()) {
    if (chunk->IsTapeDataChunk()) {
      auto tc = reinterpret_cast<const std::shared_ptr<TapeDataChunk> &>(chunk);
      auto blocks = parse_blocks(tc->Data());
      for (const auto &block : blocks) {
        auto f = files.find(block->FileName());
        std::shared_ptr<TapeFile> tf;
        if( f == files.end()) {
          tf = std::make_shared<TapeFile>();
          tf->name = block->FileName();
          tf->load_addr = block->LoadAddress();
          tf->exec_addr = block->ExecutionAddress();
          tf->data.reserve(256);
          files.emplace(tf->name, tf);
        } else {
          tf= f->second;
        }
        tf->data.insert(tf->data.end(), block->Data().begin(), block->Data().end());
      }
    }
  }
  return files;
}

std::map<std::string, std::shared_ptr<TapeFile>>
load_tape_data_from_uef(const std::string &file_name) {
  return load_tape_data_from_uef(UefData::FromFile(file_name));
}
