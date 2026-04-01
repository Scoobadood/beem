#pragma once

/**
 * Pluggable tape format interface.
 *
 * Presents the tape contents as a stream of individual bits, already
 * serialised into 8N1 frames (start bit, 8 data bits LSB-first, stop bit).
 * The stream has no knowledge of baud rate — that is the sULA's concern.
 *
 * Implementations: RawBytesTapeStream (testing), UefTapeStream (real tapes).
 */
class ITapeStream {
 public:
  virtual ~ITapeStream() = default;

  // Return the next bit on the data line and advance the stream.
  // Returns true (mark/idle) once end_of_tape() is true.
  virtual bool next_bit() = 0;

  // Accept an incoming bit (recording path).
  virtual void write_bit(bool bit) = 0;

  // True while a carrier tone is present (drives ICassettePort::has_carrier).
  virtual bool at_carrier() = 0;

  // True when all data has been consumed.
  virtual bool end_of_tape() const = 0;
};
