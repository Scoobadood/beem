#include <iostream>
#include <cstdint>
#include <cctype>
#include <spdlog/spdlog.h>
#include <UEF/uef.h>
#include "uef_tape_stream.h"

/*
 * tape-signal-dump <file.uef>
 *
 * Loads a UEF file, walks the UefTapeStream, and emits a human-readable
 * annotated signal log suitable for inspection and golden-file comparison.
 *
 * Output format (one event per line):
 *   CARRIER <n>       — n mark-bit carrier cycles
 *   GAP     <n>       — n mark-bit gap cycles
 *   BYTE    0xXX 'c'  — decoded 8N1 data byte, with printable character
 *
 * The 8N1 decoder state machine mirrors RawBytesTapeStream::write_bit():
 * it waits for a start bit (0), accumulates 8 data bits LSB-first, then
 * commits the byte on the stop bit.  Carrier and gap mark bits are ignored
 * while in the waiting-for-start state.
 */

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: tape-signal-dump <file.uef>\n";
    return EXIT_FAILURE;
  }

  spdlog::set_level(spdlog::level::warn);  // suppress UEF parse debug output

  UefData uef = UefData::FromFile(argv[1]);
  UefTapeStream stream(uef);

  // 8N1 decoder state
  int     rx_state = -1;  // -1=wait-for-start, 0-7=data bits, 8=stop
  uint8_t rx_byte  = 0;

  // Pending run lengths (flushed when segment type changes or a byte begins)
  size_t carrier_count = 0;
  size_t gap_count     = 0;

  auto flush_carrier = [&]() {
    if (carrier_count > 0) {
      std::cout << "CARRIER " << carrier_count << "\n";
      carrier_count = 0;
    }
  };
  auto flush_gap = [&]() {
    if (gap_count > 0) {
      std::cout << "GAP     " << gap_count << "\n";
      gap_count = 0;
    }
  };

  while (!stream.end_of_tape()) {
    bool bit        = stream.next_bit();
    bool is_carrier = stream.at_carrier();

    if (rx_state == -1 && bit) {
      // Mark bit outside an 8N1 frame: carrier or gap.
      if (is_carrier) {
        flush_gap();
        ++carrier_count;
      } else {
        flush_carrier();
        ++gap_count;
      }
      continue;
    }

    // Bit is either a start bit (0) or we are inside an 8N1 frame.
    flush_carrier();
    flush_gap();

    if (rx_state == -1) {
      // Start bit (must be 0 to reach here)
      rx_state = 0;
      rx_byte  = 0;
    } else if (rx_state < 8) {
      // Data bits, LSB first
      if (bit) rx_byte |= static_cast<uint8_t>(1u << rx_state);
      ++rx_state;
    } else {
      // Stop bit — emit decoded byte
      char display = std::isprint(static_cast<unsigned char>(rx_byte))
                   ? static_cast<char>(rx_byte) : '.';
      std::cout << "BYTE    0x"
                << std::hex << std::uppercase
                << (static_cast<int>(rx_byte) < 0x10 ? "0" : "")
                << static_cast<int>(rx_byte)
                << std::dec
                << " '" << display << "'\n";
      rx_state = -1;
      rx_byte  = 0;
    }
  }

  // Flush any trailing carrier or gap
  flush_carrier();
  flush_gap();

  return EXIT_SUCCESS;
}
