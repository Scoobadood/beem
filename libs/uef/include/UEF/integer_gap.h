#ifndef LIBS_UEF_INTEGER_GAP_H_
#define LIBS_UEF_INTEGER_GAP_H_

#include "chunk.h"

/**
 *   A gap in the tape - a length of time for which no sound is on the
 *   source audio casette. This chunk holds a two byte rest length counted
 *   relative to the base frequency. A value of n indicates a gap of
 *   1/(2n*base frequency) seconds.
 */
class IntegerGap : public Chunk {
 public:
  IntegerGap(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~IntegerGap() override = default;

  std::string Description() const override;
  uint16_t Duration() const { return duration_; }

 private:
  uint16_t duration_;
};
#endif // LIBS_UEF_INTEGER_GAP_H_
