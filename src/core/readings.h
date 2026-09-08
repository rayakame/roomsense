#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <mutex>

namespace roomsense {

// Latest values of all sensors. NAN means no valid value is available, the
// *_ok flags tell whether the corresponding sensor currently works.
struct Readings {
  float temperature = NAN;      // Temperature recorded by SHT45 (most accurate)
  float dps_temperature = NAN;  // Temperature recorded by DPS310
  float humidity = NAN;
  float lux = NAN;
  float pressure = NAN;
  float voc_index = NAN;
  float nox_index = NAN;
  float co2 = NAN;

  float pm_1 = NAN;
  float pm_2_5 = NAN;
  float pm_4 = NAN;
  float pm_10 = NAN;

  float noise_leq = NAN;
  float noise_max = NAN;

  bool sht_ok = false;   // Humidity & Temp
  bool veml_ok = false;  // Lux
  bool dps_ok = false;   // Pressure
  bool sgp_ok = false;   // VOx & NOx
  bool sps_ok = false;   // PM
  bool scd_ok = false;   // CO2
  bool mic_ok = false;

  uint32_t updated_at = 0;
};

// Thread-safe holder of the current Readings. Writers modify the stored values
// through Update(), readers take a copy via Snapshot().
class ReadingsStore {
 public:
  // The global store shared by all sensors and consumers.
  static ReadingsStore& Instance();

  void Update(const std::function<void(Readings&)>& updater);
  Readings Snapshot() const;

 private:
  mutable std::mutex mutex_;
  Readings data_;
};

}  // namespace roomsense
