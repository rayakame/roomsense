#pragma once

#include <cmath>

#include <Adafruit_DPS310.h>

#include "core/readings.h"
#include "sensors/sensor.h"

namespace roomsense {

// Barometric pressure sensor (Infineon DPS310, I2C). Also reports its own
// temperature reading, which is less accurate than the SHT45's.
class Dps310 : public Sensor {
 public:
  const char* Name() const override;
  bool Init() override;
  bool Read() override;
  void Apply(Readings& readings) const override;

 private:
  void Invalidate();

  Adafruit_DPS310 dps_;
  float pressure_ = NAN;
  float temperature_ = NAN;
  bool ok_ = false;
};

}  // namespace roomsense
