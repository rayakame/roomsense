#pragma once

#include <cmath>

#include <Adafruit_VEML7700.h>

#include "core/readings.h"
#include "sensors/sensor.h"

namespace roomsense {

// Ambient light sensor (Vishay VEML7700, I2C).
class Veml7700 : public Sensor {
 public:
  const char* Name() const override;
  void Apply(Readings& readings) const override;

 private:
  bool DoInit() override;
  bool DoRead() override;
  void Invalidate() override;

  Adafruit_VEML7700 veml_;
  // begin() allocates its register objects and never frees them, so it may
  // only run once.
  bool driver_started_ = false;
  float lux_ = NAN;
};

}  // namespace roomsense
