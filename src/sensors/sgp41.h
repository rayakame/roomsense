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
  bool Init() override;
  bool Read() override;
  void Apply(Readings& readings) const override;

 private:
  void Invalidate();

  SensirionI2CSgp41 sgp_;
  // Both algorithms assume one sample per second (see kSensorIntervalMs).
  VOCGasIndexAlgorithm voc_algo_;
  NOxGasIndexAlgorithm nox_algo_;
  float voc_index_ = NAN;
  float nox_index_ = NAN;
  bool ok_ = false;
};

}  // namespace roomsense
