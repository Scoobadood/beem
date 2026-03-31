#pragma once

/**
 * Interface for the external cassette player / recorder.
 *
 * Called by SerialUla on each rx/tx clock tick and on motor-relay changes.
 * The player presents one already-demodulated bit per rx_clock call —
 * it has no knowledge of baud rate; that is the sULA's concern.
 */
class ICassettePlayer {
 public:
  virtual ~ICassettePlayer() = default;

  // Called by sULA before each rx_clock tick.
  // Returns the current demodulated bit on the data line (true = mark).
  virtual bool rx_data() = 0;

  // Returns true when a carrier tone is present.
  // Used by sULA to drive ACIA DCD (active-low: clear_dcd when true).
  virtual bool has_carrier() = 0;

  // Called by sULA after each tx_clock tick with the current ACIA TX pin state.
  virtual void tx_bit(bool bit) = 0;

  // Called whenever SCR bit 7 changes (true = motor on).
  virtual void set_motor(bool on) = 0;
};
