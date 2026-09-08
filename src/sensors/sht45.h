#pragma once

#include <cmath>

#include <Adafruit_SHT4x.h>

#include "core/readings.h"
#include "sensors/sensor.h"

namespace roomsense {

// Temperature and humidity sensor (Sensirion SHT45, I2C).
class Sht45 : public Sensor {
 public:
  const char* Name() const override;
  void Apply(Readings& readings) const override;

 private:
  bool DoInit() override;
  bool DoRead() override;
  void Invalidate() override;

  Adafruit_SHT4x sht_;
  float temperature_ = NAN;
  float humidity_ = NAN;
};

}  // namespace roomsense
