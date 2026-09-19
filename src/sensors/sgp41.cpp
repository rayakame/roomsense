#include "sensors/sgp41.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "core/readings.h"
#include "core/time.h"
#include "core/voc_baseline.h"

namespace roomsense {
namespace {

// Compensation values the datasheet prescribes when no measurement is
// available: 50 %RH and 25 °C.
constexpr uint16_t kDefaultRhTicks = 0x8000;
constexpr uint16_t kDefaultTTicks = 0x6666;

// The datasheet wants conditioning at 1 Hz, and the sensor task already polls
// at that rate, so one command per cycle needs no blocking loop and leaves the
// other sensors their turn.
constexpr int kConditioningCycles = 10;

// Conditioning is a mode, not a command: the heater stays in it until the
// first measurement. Exceeding 10 s damages the sensing material, and we can
// only check at cycle boundaries, so leave one cycle of margin.
constexpr uint32_t kConditioningMaxMs = 10000 - kSensorIntervalMs;

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
  // Conditioning and the serial number are idle mode commands. After a read
  // failure the heater is still running, so stop it first. Fails harmlessly
  // when the sensor is gone; the serial number below is what detects that.
  sgp_.turnHeaterOff();

  std::array<uint16_t, 3> serial = {};
  if (sgp_.getSerialNumber(serial.data()) != 0) {
    return false;
  }

  RestoreBaseline();
  // The heater needs conditioning after every power-up, so this also runs
  // again after a reconnect. DoRead() carries it out, one command per cycle.
  conditioning_left_ = kConditioningCycles;
  conditioning_started_ = false;
  return true;
}

// One conditioning command, sent in place of a measurement.
bool Sgp41::Condition() {
  uint16_t sraw_voc = 0;
  if (sgp_.executeConditioning(kDefaultRhTicks, kDefaultTTicks, sraw_voc) != 0) {
    return false;
  }
  if (!conditioning_started_) {
    conditioning_started_ = true;
    conditioning_limit_.ArmIn(kConditioningMaxMs);
  }
  if (--conditioning_left_ == 0) {
    warmup_.ArmIn(kWarmupMs);
  }
  return true;
}

void Sgp41::RestoreBaseline() {
  if (baseline_loaded_) {
    return;
  }
  baseline_loaded_ = true;

  VocBaseline baseline;
  if (!LoadVocBaseline(baseline)) {
    // Nothing stored yet, the algorithm has to learn from scratch.
    next_baseline_save_.ArmIn(kVocBaselineMinRuntimeMs);
    return;
  }
  voc_algo_.SetStates(baseline.state0, baseline.state1);
  next_baseline_save_.ArmIn(kVocBaselineSaveIntervalMs);
  Serial.printf("SGP41: VOC baseline restored (%.1f / %.1f)\n", baseline.state0, baseline.state1);
}

void Sgp41::SaveBaseline() {
  VocBaseline baseline;
  voc_algo_.GetStates(baseline.state0, baseline.state1);
  if (SaveVocBaseline(baseline)) {
    Serial.printf("SGP41: VOC baseline saved (%.1f / %.1f)\n", baseline.state0, baseline.state1);
  } else {
    Serial.println("SGP41: saving the VOC baseline failed");
  }
}

bool Sgp41::DoRead() {
  if (conditioning_left_ > 0) {
    if (!conditioning_started_ || !conditioning_limit_.Elapsed()) {
      return Condition();
    }
    // Out of time with cycles to spare. Sending another command would only
    // extend the mode, while measuring is what ends it, so measure now.
    conditioning_left_ = 0;
    warmup_.ArmIn(kWarmupMs);
  }

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
  if (!warmup_.Elapsed()) {
    voc_index_ = NAN;
    nox_index_ = NAN;
    return true;
  }
  voc_index_ = IndexOrNan(voc_algo_.process(sraw_voc));
  nox_index_ = IndexOrNan(nox_algo_.process(sraw_nox));

  if (next_baseline_save_.Elapsed()) {
    SaveBaseline();
    next_baseline_save_.ArmIn(kVocBaselineSaveIntervalMs);
  }
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
