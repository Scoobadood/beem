#ifndef LIBS_UEF_SECURITY_CYCLES_H_
#define LIBS_UEF_SECURITY_CYCLES_H_

#include "chunk.h"

/**
 * Chunk &0114 - security cycles
 *
 * Security cycles are mainly found at the start of a run of carrier tone as an identification feature.
 * Rarely they are at the end of a run of carrier tone. They consist of cycles of the base frequency and twice the base
 * frequency and sometimes have a leading and/or trailing pulse.
 *
 * The first three bytes of this chunk (a 24 bit value) denote the number of 'cycles'.
 * It is possible that the first and last may be only pulses.
 *
 * The fourth byte holds the ASCII code for 'P' or 'W'. If it is 'P', the first cycle is
 * replaced by a single high pulse.
 *
 * The fifth byte again holds the ASCII code 'P' or 'W' which, if it is 'P' signifies that the
 * last cycle is replaced by a low pulse.
 *
 * If the fifth byte is 'P' the fourth byte must be 'W' but has no relevance.
 *
 * If the 'cycles' follow a gap then the fourth byte can logically be 'P' or 'W'. If the 'cycles'
 * follow other cycles then the fourth byte will logically be 'W'.
 *
 * This chunk never offends the general rule that the stored waveform consists only of gaps and pulses joined at
 * zero crossings, and never creates an external or internal phase change.
 *
 * The UEF is encoded with eight 'cycles' per byte.
 * Slow cycles (at the base frequency) are denoted by 0 bits.
 * Fast cycles (at twice the base frequency) are denoted by 1 bits.
 * Bits are ordered such that the most significant bit represents the first cycle on the tape.
 * Spare bits in the last byte should preferably be 0 bits.
 * When the number of cycles is 1:
 *     Only one of the fourth and fifth bytes may be 'P'
 *     If the fourth byte is 'P' the fifth byte must be 'W' but has no relevance
 *
 *
 *
 * Examples:
 * The sequence of cycles LSLLLSSLSSLLSL will be stored as &0E, &00, &00, 'W', 'W', &9D, &2C.
 * A sequence following other cycles having only 1 short pulse will be stored as &01, &00, &00, 'W', 'P', &00.
 * A sequence following a gap having only 1 short pulse followed by 3 long waves will
 * be stored as &04, &00, &00, 'P', 'W', &0E.
 */

class SecurityCycles : public Chunk {
 public:
  SecurityCycles(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~SecurityCycles() override = default;

  std::string Description() const override;
};
#endif // LIBS_UEF_SECURITY_CYCLES_H_
