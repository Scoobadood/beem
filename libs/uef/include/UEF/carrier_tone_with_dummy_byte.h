#ifndef LIBS_UEF_CARRIER_TONE_WITH_DUMMY_BYTE_H_
#define LIBS_UEF_CARRIER_TONE_WITH_DUMMY_BYTE_H_

#include "chunk.h"

/*
 * This chunk represents a run of carrier tone followed by 10 bits of data and then a second run of carrier tone.
 * This four byte chunk is composed of two sets of two bytes - the first two describe the number of cycles in the
 * tone before the dummy byte, and the second two describe the number of cycles in the tone after the dummy byte.
 * The dummy byte always has value &AA.
 */
class CarrierToneWithDummyByte : public Chunk {
 public:
  explicit CarrierToneWithDummyByte(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~CarrierToneWithDummyByte() override = default;

  std::string Description() const override;
  uint16_t PreByteCycleCount()  const { return pre_byte_cycle_count_; }
  uint16_t PostByteCycleCount() const { return post_byte_cycle_count_; }

 private:
  uint16_t pre_byte_cycle_count_;
  uint16_t post_byte_cycle_count_;
};
#endif // LIBS_UEF_CARRIER_TONE_WITH_DUMMY_BYTE_H_
