#include "chunk.h"
#include "unknown_chunk.h"
#include "origin_chunk.h"
#include "carrier_tone.h"
#include "stream_utils.h"
#include "tape_data_block.h"
#include "integer_gap.h"
#include "multiplexed_tape_data_block.h"
#include "explicit_tape_data_block.h"
#include "custom_tape_data_block.h"
#include "carrier_tone_with_dummy_byte.h"
#include "floating_point_gap.h"
#include "phase_change.h"
#include "frequency_change.h"
#include "security_cycles.h"

#include <iostream>
#include <spdlog/spdlog.h>


enum ChunkType {
  CT_ORIGIN = 0x0000,
  CT_MANUAL = 0x0001,
  CT_INLAY_SCAN = 0x0003,
  CT_TARGET_MACHINE = 0x0005,
  CT_BIT_MULTPLEXING = 0x0006,
  CT_EXTRA_PALETTE = 0x0007,
  CT_ROM_HINT = 0x0008,
  CT_SHORT_TITLE = 0x0009,
  CT_VISIBLE_AREA = 0x0010,
  CT_DATA_BLOCK = 0x0100,
  CT_MULTIPLEXED_TAPE_DATA_BLOCK = 0x0101,
  CT_EXPLICIT_TAPE_DATA_BLOCK = 0x0102,
  CT_CUSTOM_TAPE_DATA_BLOCK = 0x0104,
  CT_CARRIER_TONE = 0x0110,
  CT_CARRIER_TONE_WITH_DUMMY_BYTE = 0x0111,
  CT_INTEGER_GAP = 0x0112,
  CT_FLOATING_POINT_GAP = 0x0116,
  CT_FREQUENCY_CHANGE = 0x0113,
  CT_SECURITY_CYCLES = 0x0114,
  CT_PHASE_CHANGE = 0x0115,
  CT_DATA_ENCODING_FORMAT_CHANGE = 0x0117,
  CT_POSITION_MARKER = 0x0120,
  CT_TAPE_SET_INFORMATION = 0x0130,
  CT_START_OF_TAPE_SIDE = 0x0131,
  CT_EMULATOR_ID = 0xff00
};

Chunk::Chunk(std::string chunk_type)
    : chunk_type_{std::move(chunk_type)} //
{}

const std::string &
Chunk::TypeName() const {
  return chunk_type_;
}

std::ostream &operator<<(std::ostream &os, const Chunk &obj) {
  os << obj.Description();
  return os;
}

std::shared_ptr<Chunk>
Chunk::ParseChunk(std::unique_ptr<std::istream> &uef_stream) {
  auto chunk_id = read_uint_16(uef_stream);
  auto chunk_length = read_uint_32(uef_stream);
  auto current_pos = uef_stream->tellg();

  std::shared_ptr<Chunk> chunk;
  switch (chunk_id) {
    case CT_ORIGIN:
      chunk = std::make_shared<OriginChunk>(uef_stream, chunk_length);
      break;
    case CT_CARRIER_TONE:
      chunk = std::make_shared<CarrierTone>(uef_stream, chunk_length);
      break;
    case CT_CARRIER_TONE_WITH_DUMMY_BYTE:
      chunk = std::make_shared<CarrierToneWithDummyByte>(uef_stream, chunk_length);
      break;
    case CT_DATA_BLOCK:
      chunk = std::make_shared<TapeDataBlock>(uef_stream, chunk_length);
      break;
    case CT_INTEGER_GAP:
      chunk = std::make_shared<IntegerGap>(uef_stream, chunk_length);
      break;
    case CT_FLOATING_POINT_GAP:
      chunk = std::make_shared<FloatingPointGap>(uef_stream, chunk_length);
      break;
    case CT_MULTIPLEXED_TAPE_DATA_BLOCK:
      chunk = std::make_shared<MultiplexedTapeDataBlock>(uef_stream, chunk_length);
      break;
    case CT_EXPLICIT_TAPE_DATA_BLOCK:
      chunk = std::make_shared<ExplicitTapeDataBlock>(uef_stream, chunk_length);
      break;
    case CT_CUSTOM_TAPE_DATA_BLOCK:
      chunk = std::make_shared<CustomTapeDataBlock>(uef_stream, chunk_length);
      break;
    case CT_PHASE_CHANGE:
      chunk = std::make_shared<PhaseChange>(uef_stream, chunk_length);
      break;
    case CT_FREQUENCY_CHANGE:
      chunk = std::make_shared<FrequencyChange>(uef_stream, chunk_length);
      break;
    case CT_SECURITY_CYCLES:
      chunk = std::make_shared<SecurityCycles>(uef_stream, chunk_length);
      break;
    default:
      chunk = std::make_shared<UnknownChunk>(chunk_id);
      uef_stream->seekg(chunk_length, std::ios::seekdir::cur);
      break;
  }

  auto read_length = uef_stream->tellg() - current_pos;
  if (read_length != chunk_length) {
    spdlog::error("Expected to read {} bytes but actually read {} in chunk {}",
                  chunk_length, read_length, chunk->TypeName());
  }
  return chunk;
}
