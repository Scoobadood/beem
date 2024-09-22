#ifndef LIBS_UEF_PHASE_CHANGE_H_
#define LIBS_UEF_PHASE_CHANGE_H_

#include "chunk.h"

/**
 * Chunk &0115 - phase change
 * This chunk contains a 16 bit unsigned value between 0 and 359, which determines the new phase.
 *
 * The majority of professional cassettes have waves shifted 0 or 180 degrees. Before one of these
 * chunks is met (i.e. immediately after opening a UEF) the phase shift should be taken to be 180 degrees.
 *
 * If accurately representing a real life source tape, this chunk will only be found neighbouring a gap.
 *
 * See the section entitled 'Notes on phase' towards the top of this document for a proper discussion of
 * the effect of phase on the output waveform.
 */
class PhaseChange : public Chunk {
 public:
  PhaseChange(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~PhaseChange() override = default;

  std::string Description() const override;

 private:
  uint16_t new_phase_;
};

#endif // LIBS_UEF_PHASE_CHANGE_H_
