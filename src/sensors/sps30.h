#pragma once

#include <cmath>

#include "core/readings.h"
#include "sensors/sensor.h"

namespace roomsense {

// Particulate matter sensor (Sensirion SPS30, I2C). Reports mass
// concentrations for PM1.0, PM2.5, PM4.0 and PM10 in µg/m³.
class Sps30 : public Sensor {
 public:
  const char* Name() const override;
  bool Init() override;
  bool Read() override;
  void Apply(Readings& readings) const override;

 private:
  void Invalidate();

  float mc_1p0_ = NAN;
  float mc_2p5_ = NAN;
  float mc_4p0_ = NAN;
  float mc_10p0_ = NAN;
  bool ok_ = false;
};

}  // namespace roomsense
