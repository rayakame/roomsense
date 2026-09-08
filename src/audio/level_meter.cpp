#include "audio/level_meter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>

#include "audio/sos_filter.h"

#include "config.h"
#include "core/readings.h"

namespace roomsense {
namespace {

// Beide Filtersaetze fuer 48 kHz aus esp32-i2s-slm (Ivan Kostoski).
// Entzerrung des ICS-43434-Frequenzgangs.
constexpr float kMicEqGain = 0.477326418836803f;
constexpr std::array<SosSection, 2> kMicEqSections = {{
    {.b1 = +0.96986791463971267f,
     .b2 = +0.23515976355743193f,
     .a1 = -0.06681948004769928f,
     .a2 = -0.00111521990688128f},
    {.b1 = -1.98905931743624453f,
     .b2 = +0.98908924206960169f,
     .a1 = +1.99755331853906037f,
     .a2 = -0.99755481510122113f},
}};

// A-Bewertung nach IEC 61672.
constexpr float kAWeightingGain = 0.169994948147430f;
constexpr std::array<SosSection, 3> kAWeightingSections = {{
    {.b1 = -2.00026996133106f,
     .b2 = +1.00027056142719f,
     .a1 = -1.060868438509278f,
     .a2 = -0.163987445885926f},
    {.b1 = +4.35912384203144f,
     .b2 = +3.09120265783884f,
     .a1 = +1.208419926363593f,
     .a2 = -0.273166998428332f},
    {.b1 = -0.70930303489759f,
     .b2 = -0.29071868393580f,
     .a1 = +1.982242159753048f,
     .a2 = -0.982298594928989f},
}};

// Umrechnung von Signalleistung in dB SPL.
constexpr float kMicSensitivityDb = -26.0f;  // dBFS bei 94 dB SPL (Datenblatt ICS-43434)
constexpr float kMicRefDb = 94.0f;
constexpr float kMicOffsetDb = 3.0103f;      // Sinus-RMS vs. Vollausschlag
constexpr float kFullScale = (1 << 23) - 1;  // 24 Bit

float ToDb(double power) {
  static const float kRefAmpl = powf(10.0f, kMicSensitivityDb / 20.0f) * kFullScale;
  const float rms = sqrtf(static_cast<float>(power));
  return kMicOffsetDb + kMicRefDb + (20.0f * log10f(rms / kRefAmpl));
}

}  // namespace

LevelMeter::LevelMeter()
    : mic_eq_(kMicEqGain, kMicEqSections), a_weighting_(kAWeightingGain, kAWeightingSections) {}

void LevelMeter::Process(std::span<const int32_t> block) {
  // 24-Bit-Sample aus dem 32-Bit-Slot holen und nach float wandeln.
  for (std::size_t i = 0; i < buffer_.size(); ++i) {
    buffer_[i] = static_cast<float>(block[i] >> 8);
  }

  mic_eq_.Process(buffer_);
  a_weighting_.Process(buffer_);

  // Mittlere Leistung des Blocks. double, weil 6000 Quadrate grosser Werte
  // float sprengen wuerden.
  double power = 0;
  for (const float sample : buffer_) {
    power += static_cast<double>(sample) * sample;
  }
  power /= static_cast<double>(buffer_.size());

  sum_power_ += power;
  max_power_ = std::max(max_power_, power);
  ++blocks_;

  if (blocks_ >= kMicBlocksPerSecond) {
    Publish();
    sum_power_ = 0;
    max_power_ = 0;
    blocks_ = 0;
  }
}

void LevelMeter::Publish() const {
  const float leq = ToDb(sum_power_ / blocks_);
  const float lmax = ToDb(max_power_);
  ReadingsStore::Instance().Update([leq, lmax](Readings& readings) {
    readings.noise_leq = leq;
    readings.noise_max = lmax;
    readings.mic_ok = true;
  });
}

void LevelMeter::Invalidate() {
  sum_power_ = 0;
  max_power_ = 0;
  blocks_ = 0;
  ReadingsStore::Instance().Update([](Readings& readings) {
    readings.noise_leq = NAN;
    readings.noise_max = NAN;
    readings.mic_ok = false;
  });
}

}  // namespace roomsense