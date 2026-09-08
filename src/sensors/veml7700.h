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
  bool Init() override;
  bool Read() override;
  void Apply(Readings& readings) const override;

 private:
  Adafruit_VEML7700 veml_;
  float lux_ = NAN;
  bool ok_ = false;
};

}  // namespace roomsense
