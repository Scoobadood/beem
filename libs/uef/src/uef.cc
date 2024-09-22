#include "uef.h"

#include <utility>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "zlib.h"
#include "spdlog/spdlog.h"

const char *UEF_HEADER = "UEF File!\0";

/*
 * Identify a gzip file by its header.
 * Magic Number: The first two bytes of a gzip file are always 0x1F 0x8B.
 * Compression Method: The third byte represents the compression method, typically 0x08, which stands for deflate
 */
bool is_gzip_archive(std::unique_ptr<std::ifstream> &uef_file) {
  uint8_t gzip_header[3];
  uef_file->read(reinterpret_cast<char *>(gzip_header), 3);
  auto pos = uef_file->gcount();
  uef_file->seekg(0, std::ios::seekdir::beg);

  if (pos != 3) {
    return false;
  }

  return ((gzip_header[0] == 0x1F) && (gzip_header[1] == 0x8B) && (gzip_header[2] == 0x08));
}

int decompress_gzip(const std::vector<uint8_t> &compressed_data, std::vector<uint8_t> &uncompressed_data) {
  z_stream strm = {};
  strm.next_in = const_cast<Bytef *>(compressed_data.data());
  strm.avail_in = compressed_data.size();

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  // Initialize with 16 + MAX_WBITS to support gzip headers
  int ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK) {
    return ret;
  }

  // Reserve space in the uncompressed buffer
  uncompressed_data.resize(compressed_data.size() * 2);  // Start with 2x the compressed size

  strm.next_out = uncompressed_data.data();
  strm.avail_out = uncompressed_data.size();

  uint32_t total_out = 0;

  // Decompress in a loop
  do {
    if (strm.avail_out == 0) {
      // Expand the output buffer if necessary
      size_t current_size = uncompressed_data.size();
      uncompressed_data.resize(current_size * 2);
      strm.next_out = uncompressed_data.data() + current_size;
      strm.avail_out = current_size;
    }
    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret == Z_DATA_ERROR) {
      std::cerr << "Data error during decompression" << std::endl;
      break;
    }
    // We may have multiple concatenated GZip segments.
    if (ret == Z_STREAM_END && strm.avail_in != 0) {
      total_out += strm.total_out;
      // Reinitialize the stream to handle the next compressed stream
      ret = inflateReset2(&strm, 16 + MAX_WBITS);
      if (ret != Z_OK) {
        inflateEnd(&strm);
        total_out += strm.total_out;
        return ret;
      }
    }
  } while (ret != Z_STREAM_END);

  inflateEnd(&strm);
  total_out += strm.total_out;

  if (ret == Z_STREAM_END) {
    // Resize the output buffer to the actual uncompressed size
    uncompressed_data.resize(total_out);
    return Z_OK;
  }

  return ret;  // Return the error code if it was not successful
}

std::unique_ptr<std::istream>
unzip_file_to_stream(std::unique_ptr<std::ifstream> &uef_file) {
  std::cout << "File is GZipped" << std::endl;

  uef_file->seekg(0, std::ios::seekdir::end);
  auto file_size = uef_file->tellg();
  uef_file->seekg(0, std::ios::beg);

  std:: cout << "        zipped length: "  << std::dec << file_size << " bytes"
                                     << "(0x" << std::hex << std::setw(10) << std::setfill('0')
                                     << file_size << ")" << std::endl;

  // Allocate a buffer large enough to hold the zipped file
  std::vector<uint8_t> buffer(file_size);
  if (!uef_file->read(reinterpret_cast<char *>(buffer.data()), file_size)) {
    throw std::runtime_error("Error reading file");
  }

  // Allocate output vector
  std::vector<uint8_t> output;
  decompress_gzip(buffer, output);

  std::cout << "  uncompressed length: "  << std::dec << file_size << " bytes"
            << "(0x" << std::hex << std::setw(10) << std::setfill('0')
            << file_size << ")" << std::endl;

  // Wrap the output into a stream and return it
  std::string str(output.begin(), output.end());




  return std::unique_ptr<std::istream>(new std::istringstream(str));
}

/*
 * Read header or throw.
 * Introduction
 * Files begin with a 12 byte file header:
 * 10 bytes: Null terminated string "UEF File!"
 * 1 byte: minor version number
 * 1 byte: major version number
 */
void check_header(std::unique_ptr<std::istream> &uef_stream) {
  for (auto i = 0; i < strlen(UEF_HEADER) + 1; i++) {
    char c;
    uef_stream->get(c);
    if (c != UEF_HEADER[i]) {
      throw std::runtime_error("Not a valid UEF File - invalid header");
    }
  }
}

/*
 * Read single byte major and minor versions
 */
void read_version(std::unique_ptr<std::istream> &uef_stream, int8_t &major, int8_t &minor) {
  uef_stream->get((char &) major);
  uef_stream->get((char &) minor);
}

/**
 * Read the list of Chunks
 * @param uef_stream
 * @return
 */
void UefData::ReadChunks(std::unique_ptr<std::istream> &uef_stream) {
  using namespace std;

  /*
   * Iterate over all of the chunks
   * describing them
   */
  while (uef_stream->peek() != char_traits<char>::eof()) {
    auto chunk = Chunk::ParseChunk(uef_stream);
    chunks_.push_back(chunk);
  }
}

/**
 * Construct from a stream
 *
 * @param uef_stream
 */
UefData::UefData(std::unique_ptr<std::istream> &uef_stream)
    : major_version_{0} //
    , minor_version_{0} //
{
  check_header(uef_stream);
  read_version(uef_stream, major_version_, minor_version_);
  ReadChunks(uef_stream);
}

UefData UefData::FromStream(std::unique_ptr<std::istream> &uef_stream) {
  return UefData{uef_stream};
}

UefData UefData::FromFile(const std::string &file_name) {
  // Load the UEF file and spit out the names of all of the chunks

  auto uef_file = std::unique_ptr<std::ifstream>(new std::ifstream(file_name, std::ios::binary));
  if (!uef_file->is_open()) {
    auto msg = fmt::format("Failed to open file {}", file_name);
    spdlog::error(msg);
    throw std::runtime_error(msg);
  }
  std::cout << "Reading from file " << file_name << std::endl;

  // This file may be a gzip archive in which case unzip it first
  std::unique_ptr<std::istream> uef_stream;
  if (is_gzip_archive(uef_file)) {
    uef_stream = unzip_file_to_stream(uef_file);
  } else {
    uef_file->clear();
    uef_file->seekg(0, std::ios::beg);
    uef_stream = std::move(uef_file);  // Move the ifstream into the generic istream
  }

  return UefData::FromStream(uef_stream);
}

const std::vector<std::shared_ptr<Chunk>> &
UefData::Chunks() const {
  return chunks_;
}