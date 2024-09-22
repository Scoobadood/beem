#ifndef LIBS_UEF_FLOATING_POINT_GAP_H_
#define LIBS_UEF_FLOATING_POINT_GAP_H_

#include "chunk.h"

#include <string>

class FloatingPointGap : public Chunk {
 public:
  FloatingPointGap(std::unique_ptr<std::istream> &uef_stream, uint32_t chunk_length);
  ~FloatingPointGap() override = default;

  std::string Description() const override;

 private:
  float duration_;
};


#endif // LIBS_UEF_FLOATING_POINT_GAP_H_
