#pragma once

#include <cmath>

#include "core/readings.h"
#include "sensors/sensor.h"

namespace roomsense {

class Sps30 : public Sensor {
 public:
  const char* Name() const override;
  void Apply(Readings& readings) const override;

 private:
  bool DoInit() override;
  bool DoRead() override;
  void Invalidate() override;

  bool bus_initialized_ = false;
  int polls_without_data_ = 0;
  float mc_1p0_ = NAN;
  float mc_2p5_ = NAN;
  float mc_4p0_ = NAN;
  float mc_10p0_ = NAN;
};

}  // namespace roomsense
