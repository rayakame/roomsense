#include "sensors/sgp41.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <Arduino.h>
#include <Wire.h>

#include "core/readings.h"
#include "core/time.h"

namespace roomsense {
namespace {

// Compensation values the datasheet prescribes when no measurement is
// available: 50 %RH and 25 °C.
constexpr uint16_t kDefaultRhTicks = 0x8000;
constexpr uint16_t kDefaultTTicks = 0x6666;

// The datasheet limits conditioning to 10 s.
constexpr int kConditioningSeconds = 10;

// After conditioning the raw signals still drift while the heater settles.
// Feeding them into the gas index algorithms would corrupt the learned
// baseline, so the indices stay NAN for this long after every init.
constexpr uint32_t kWarmupMs = 60000;

uint16_t RhToTicks(float humidity) {
  return static_cast<uint16_t>(std::clamp(humidity, 0.0f, 100.0f) * 65535.0f / 100.0f);
}

uint16_t TemperatureToTicks(float temperature) {
  return static_cast<uint16_t>((std::clamp(temperature, -45.0f, 130.0f) + 45.0f) * 65535.0f /
                               175.0f);
}

// The gas index algorithms report 0 while they are still warming up.
float IndexOrNan(int32_t index) { return index == 0 ? NAN : static_cast<float>(index); }

}  // namespace

const char* Sgp41::Name() const { return "SGP41"; }

bool Sgp41::DoInit() {
  sgp_.begin(Wire);

  std::array<uint16_t, 3> serial = {};
  if (sgp_.getSerialNumber(serial.data()) != 0) {
    return false;
  }

  // The heater needs conditioning after every power-up, so this also runs
  // again after a reconnect.
  for (int i = 0; i < kConditioningSeconds; i++) {
    uint16_t sraw_voc = 0;
    if (sgp_.executeConditioning(kDefaultRhTicks, kDefaultTTicks, sraw_voc) != 0) {
      return false;
    }
    delay(1000);
  }
  warmup_until_ = millis() + kWarmupMs;
  return true;
}

bool Sgp41::DoRead() {
  // Compensate with the latest SHT45 values, fall back to the defaults while
  // none are available.
  uint16_t rh_ticks = kDefaultRhTicks;
  uint16_t t_ticks = kDefaultTTicks;
  const Readings current = ReadingsStore::Instance().Snapshot();
  if (!std::isnan(current.humidity) && !std::isnan(current.temperature)) {
    rh_ticks = RhToTicks(current.humidity);
    t_ticks = TemperatureToTicks(current.temperature);
  }

  uint16_t sraw_voc = 0;
  uint16_t sraw_nox = 0;
  if (sgp_.measureRawSignals(rh_ticks, t_ticks, sraw_voc, sraw_nox) != 0) {
    return false;
  }
  if (!Reached(millis(), warmup_until_)) {
    voc_index_ = NAN;
    nox_index_ = NAN;
    return true;
  }
  voc_index_ = IndexOrNan(voc_algo_.process(sraw_voc));
  nox_index_ = IndexOrNan(nox_algo_.process(sraw_nox));
  return true;
}

void Sgp41::Apply(Readings& readings) const {
  readings.voc_index = voc_index_;
  readings.nox_index = nox_index_;
  readings.sgp_ok = IsOk();
}

void Sgp41::Invalidate() {
  voc_index_ = NAN;
  nox_index_ = NAN;
}

}  // namespace roomsense
