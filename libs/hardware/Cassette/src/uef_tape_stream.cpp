#include "uef_tape_stream.h"

#include "UEF/carrier_tone.h"
#include "UEF/carrier_tone_with_dummy_byte.h"
#include "UEF/integer_gap.h"
#include "UEF/floating_point_gap.h"
#include "UEF/tape_data_block.h"
#include "UEF/explicit_tape_data_block.h"

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::vector<bool> UefTapeStream::expand_8n1(const std::vector<uint8_t>& bytes) {
  std::vector<bool> bits;
  bits.reserve(bytes.size() * 10);
  for (uint8_t byte : bytes) {
    bits.push_back(false);              // start bit
    for (int i = 0; i < 8; ++i)
      bits.push_back((byte >> i) & 1); // data bits, LSB first
    bits.push_back(true);              // stop bit
  }
  return bits;
}

std::vector<bool> UefTapeStream::expand_raw(const std::vector<uint8_t>& bytes,
                                             uint32_t num_bits) {
  std::vector<bool> bits;
  bits.reserve(num_bits);
  // Per UEF spec 0x0102: LSB of each byte is the first bit on tape
  for (uint32_t i = 0; i < num_bits; ++i)
    bits.push_back((bytes[i / 8] >> (i % 8)) & 1);
  return bits;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void UefTapeStream::build_segments(const UefData& uef) {
  for (const auto& chunk : uef.Chunks()) {
    if (auto* tone = dynamic_cast<CarrierTone*>(chunk.get())) {
      if (tone->CycleCount() > 0)
        segments_.push_back({Segment::Kind::CARRIER, {}, tone->CycleCount()});

    } else if (auto* tdwb = dynamic_cast<CarrierToneWithDummyByte*>(chunk.get())) {
      if (tdwb->PreByteCycleCount() > 0)
        segments_.push_back({Segment::Kind::CARRIER, {}, tdwb->PreByteCycleCount()});
      segments_.push_back({Segment::Kind::DATA, expand_8n1({0xAA}), 0});
      if (tdwb->PostByteCycleCount() > 0)
        segments_.push_back({Segment::Kind::CARRIER, {}, tdwb->PostByteCycleCount()});

    } else if (auto* tdb = dynamic_cast<TapeDataBlock*>(chunk.get())) {
      auto bits = expand_8n1(tdb->Data());
      if (!bits.empty())
        segments_.push_back({Segment::Kind::DATA, std::move(bits), 0});

    } else if (auto* etdb = dynamic_cast<ExplicitTapeDataBlock*>(chunk.get())) {
      if (etdb->NumBits() > 0) {
        auto bits = expand_raw(etdb->Data(), etdb->NumBits());
        segments_.push_back({Segment::Kind::DATA, std::move(bits), 0});
      }

    } else if (auto* igap = dynamic_cast<IntegerGap*>(chunk.get())) {
      if (igap->Duration() > 0)
        segments_.push_back({Segment::Kind::GAP, {}, igap->Duration()});

    } else if (auto* fgap = dynamic_cast<FloatingPointGap*>(chunk.get())) {
      auto count = static_cast<size_t>(fgap->Duration() * 1200.0f);
      if (count > 0)
        segments_.push_back({Segment::Kind::GAP, {}, count});
    }
    // All other chunk types are silently skipped
  }

  // Initialise last_carrier_ from the first non-empty segment
  last_carrier_ = !segments_.empty() &&
                  segments_[0].kind == Segment::Kind::CARRIER;
}

UefTapeStream::UefTapeStream(const UefData& uef) {
  build_segments(uef);
}

// ---------------------------------------------------------------------------
// ITapeStream
// ---------------------------------------------------------------------------

size_t UefTapeStream::segment_size(const Segment& s) const {
  return s.kind == Segment::Kind::DATA ? s.bits.size() : s.count;
}

bool UefTapeStream::end_of_tape() const {
  return seg_idx_ >= segments_.size();
}

bool UefTapeStream::at_carrier() {
  // Returns the carrier state of the bit most recently returned by next_bit().
  // Returns false when the tape is exhausted.
  return !end_of_tape() && last_carrier_;
}

bool UefTapeStream::next_bit() {
  if (end_of_tape()) {
    last_carrier_ = false;
    return true;
  }

  const Segment& seg = segments_[seg_idx_];

  // Record carrier state for this bit; at_carrier() will reflect this value.
  last_carrier_ = (seg.kind == Segment::Kind::CARRIER);

  bool bit = (seg.kind == Segment::Kind::DATA) ? seg.bits[bit_pos_] : true;

  if (++bit_pos_ >= segment_size(seg)) {
    ++seg_idx_;
    bit_pos_ = 0;
  }

  return bit;
}
