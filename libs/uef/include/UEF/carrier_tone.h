#ifndef LIBS_UEF_CARRIER_CHUNK_H_
#define LIBS_UEF_CARRIER_CHUNK_H_

#include "chunk.h"

/*
 * A run of carrier tone (i.e. cycles with a frequency of twice the base frequency), with a running length described in cycles by the first two bytes.
 * PSEUDO-CODE (NB: see phase notes at head of document)
 * read cycle count for chunk - first two bytes, store to CycleCount
 * while CycleCount > 0
 *    output a single cycle at twice the current base frequency
 *    decrement WaveCount
 *
 * [Previously defined as A run of high (2400Hz on a 1200 baud cassette, 600Hz on a 300 baud cassette)
 * tone, with a running length described in ms by the first two bytes.
 */
class CarrierTone : public Chunk {
 public:
  explicit CarrierTone(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~CarrierTone() override = default;

  std::string Description() const override;

 private:
  uint16_t cycle_count_;
};
#endif // LIBS_UEF_CARRIER_CHUNK_H_
