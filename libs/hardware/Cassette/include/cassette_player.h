#pragma once

#include "i_cassette_player.h"
#include "i_tape_stream.h"

/**
 * Concrete ICassettePlayer.
 *
 * Bridges the sULA's rx/tx clock ticks and motor relay changes to an
 * ITapeStream. When the motor is off every call returns idle state and
 * stream calls are suppressed.
 *
 * The stream pointer is non-owning; the caller is responsible for lifetime.
 * Passing nullptr is safe — the player behaves as if the tape is absent.
 */
class CassettePlayer : public ICassettePlayer {
 public:
  CassettePlayer() = default;

  void set_stream(ITapeStream* stream) { stream_ = stream; }

  // ICassettePlayer
  bool rx_data()          override;
  bool has_carrier()      override;
  void tx_bit(bool bit)   override;
  void set_motor(bool on) override;

  bool motor_on() const { return motor_on_; }

 private:
  ITapeStream* stream_{nullptr};
  bool         motor_on_{false};
};
