#pragma once

/**
 * Represents the BBC Micro's cassette port (5-pin DIN connector).
 *
 * Any device connected to that port — real cassette player, WAV player,
 * FSK modem — implements this interface.
 *
 * In real hardware the sULA FSK-encodes/decodes between this port and the
 * ACIA (RX: audio → zero-crossing detection → bit; TX: bit → 1200/2400 Hz
 * tone). This interface operates at the demodulated bit level (post-decode
 * for RX, pre-encode for TX), because the FSK layer is format-specific and
 * lives inside ITapeStream implementations (e.g. WavTapeStream for real
 * tape recordings). No changes to SerialUla or Acia are required to add
 * new format support.
 *
 * Sets up a natural parallel with IRS423Port (future RS423 serial port).
 *
 * Called by SerialUla on each rx/tx clock tick and on motor-relay changes.
 */
class ICassettePort {
 public:
  virtual ~ICassettePort() = default;

  // Called by sULA before each rx_clock tick.
  // Returns the demodulated bit on the data line (true = mark).
  // In hardware: the bit the sULA would extract from the FSK audio signal.
  virtual bool rx_data() = 0;

  // Returns true when a carrier tone is present on the tape.
  // Used by sULA to drive ACIA DCD (active-low: clear_dcd when true).
  virtual bool has_carrier() = 0;

  // Called by sULA after each tx_clock tick with the current ACIA TX pin state.
  // In hardware: the sULA would FSK-encode this bit as a 1200/2400 Hz tone.
  virtual void tx_bit(bool bit) = 0;

  // Called whenever SCR bit 7 changes (true = motor on).
  virtual void set_motor(bool on) = 0;
};
