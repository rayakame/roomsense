#pragma once

#include <cmath>

#include <NOxGasIndexAlgorithm.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>

#include "core/readings.h"
#include "sensors/sensor.h"

namespace roomsense {

// VOC and NOx gas sensor (Sensirion SGP41, I2C). Raw signals are converted to
// the Sensirion gas indices (1..500). Temperature and humidity for
// compensation are taken from the SHT45 values in ReadingsStore.
class Sgp41 : public Sensor {
 public:
  const char* Name() const override;
  void Apply(Readings& readings) const override;

 private:
  bool DoInit() override;
  bool DoRead() override;
  void Invalidate() override;

  SensirionI2CSgp41 sgp_;
  // Both algorithms assume one sample per second (see kSensorIntervalMs).
  VOCGasIndexAlgorithm voc_algo_;
  NOxGasIndexAlgorithm nox_algo_;
  // Raw signals are not fed into the algorithms before this time, see DoInit().
  uint32_t warmup_until_ = 0;
  float voc_index_ = NAN;
  float nox_index_ = NAN;
};

}  // namespace roomsense
