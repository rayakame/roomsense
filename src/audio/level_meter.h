#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "audio/sos_filter.h"

#include "config.h"

namespace roomsense {

// Berechnet aus Rohbloecken den A-bewerteten Schallpegel und schreibt einmal
// pro Sekunde Leq und Lmax in den ReadingsStore.
class LevelMeter {
 public:
  LevelMeter();

  // Verarbeitet einen Block mit genau kMicBlockSamples Samples (24 Bit in
  // 32-Bit-Slots, wie vom I2S-Treiber geliefert). `block` bleibt unveraendert.
  void Process(std::span<const int32_t> block);

  // Meldet dem Store, dass keine gueltigen Werte vorliegen.
  void Invalidate();

 private:
  void Publish() const;

  SosFilter<2> mic_eq_;
  SosFilter<3> a_weighting_;
  std::array<float, kMicBlockSamples> buffer_{};
  double sum_power_ = 0;
  double max_power_ = 0;
  int blocks_ = 0;
};

}  // namespace roomsense