#ifndef LIBS_UEF_FREQUENCY_CHANGE_H_
#define LIBS_UEF_FREQUENCY_CHANGE_H_

#include "chunk.h"

/**
 * Chunk &0113 - change of base frequency
 * The base frequency is a modal value, which is assumed to be 1200Hz when a UEF is open.
 * If this chunk is encountered, the base frequency changes.
 *
 * This chunks contains a single floating point number, stating the new base frequency.
 */
class FrequencyChange : public Chunk {
 public:
  FrequencyChange(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~FrequencyChange() override = default;

  std::string Description() const override;

 private:
  float new_frequency_;
};

#endif // LIBS_UEF_FREQUENCY_CHANGE_H_
