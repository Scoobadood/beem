#pragma once

#include "i_tape_stream.h"
#include "UEF/uef.h"

#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * ITapeStream implementation that walks UEF chunks and emits a bit stream.
 *
 * Pre-processes UEF chunks at construction into a flat list of segments:
 *   DATA    — bytes expanded to 8N1 frames (or raw bits for 0x0102)
 *   CARRIER — mark bits, at_carrier() = true
 *   GAP     — mark bits, at_carrier() = false
 *
 * Chunk types handled:
 *   0x0100 TapeDataBlock          — 8N1 framed bytes
 *   0x0102 ExplicitTapeDataBlock  — raw bits, LSB-first per byte
 *   0x0110 CarrierTone            — carrier, count = CycleCount()
 *   0x0111 CarrierToneWithDummyByte — carrier + 0xAA (8N1) + carrier
 *   0x0112 IntegerGap             — gap, count = Duration()
 *   0x0116 FloatingPointGap       — gap, count = Duration() * 1200
 *   All others                    — silently skipped
 *
 * write_bit() is a no-op — recording to UEF is out of scope.
 */
class UefTapeStream : public ITapeStream {
 public:
  explicit UefTapeStream(const UefData& uef);

  // ITapeStream
  bool next_bit()          override;
  void write_bit(bool bit) override {}  // no-op
  bool at_carrier()        override;
  bool end_of_tape() const override;

 private:
  struct Segment {
    enum class Kind { DATA, CARRIER, GAP };
    Kind              kind;
    std::vector<bool> bits;    // DATA: pre-expanded
    size_t            count{0};// CARRIER/GAP: mark-bit count
  };

  static std::vector<bool> expand_8n1(const std::vector<uint8_t>& bytes);
  static std::vector<bool> expand_raw(const std::vector<uint8_t>& bytes, uint32_t num_bits);

  void build_segments(const UefData& uef);
  size_t segment_size(const Segment& s) const;

  std::vector<Segment> segments_;
  size_t seg_idx_{0};
  size_t bit_pos_{0};
  bool   last_carrier_{false};
};
