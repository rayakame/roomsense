#pragma once

#include <cmath>

#include <NOxGasIndexAlgorithm.h>
#include <SensirionI2CSgp41.h>

#include "core/readings.h"
#include "core/time.h"
#include "sensors/sensor.h"
#include "sensors/voc_algorithm.h"

namespace roomsense {

class Sgp41 : public Sensor {
 public:
  const char* Name() const override;
  void Apply(Readings& readings) const override;

 private:
  bool DoInit() override;
  bool DoRead() override;
  void Invalidate() override;

  void RestoreBaseline();
  void SaveBaseline();

  SensirionI2CSgp41 sgp_;
  // Both algorithms assume one sample per second (see kSensorIntervalMs).
  VocAlgorithm voc_algo_;
  NOxGasIndexAlgorithm nox_algo_;
  // The algorithms see no raw signals before this has passed.
  Deadline warmup_;
  // Loaded once. After a re-init the algorithm knows more than NVS does.
  bool baseline_loaded_ = false;
  Deadline next_baseline_save_;
  float voc_index_ = NAN;
  float nox_index_ = NAN;
};

}  // namespace roomsense
