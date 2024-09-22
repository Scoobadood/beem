#ifndef LIBS_UEF_CHUNK_H_
#define LIBS_UEF_CHUNK_H_

#include <iostream>

const int DEFAULT_BASE_FREQUENCY = 1200;

class Chunk {
 public:
  virtual ~Chunk() = default;

  virtual const std::string &TypeName() const;
  virtual std::string Description() const = 0;

  virtual bool IsTapeDataChunk() const { return false;}

  static std::shared_ptr<Chunk> ParseChunk(std::unique_ptr<std::istream> &uef_stream);

 protected:
  explicit Chunk(std::string chunk_type);

 private:
  std::string chunk_type_;
};

std::ostream& operator<<(std::ostream& os, const Chunk& obj);

#endif // LIBS_UEF_CHUNK_H_