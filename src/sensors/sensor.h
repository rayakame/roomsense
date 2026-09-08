#pragma once

#include "core/readings.h"

namespace roomsense {

// Common interface of all sensor drivers.
//
// Init() is called once at startup and returns false if the hardware is not
// usable. Read() takes one measurement and returns false on failure, in which
// case the sensor holds no valid values until the next successful Read().
// Apply() copies the last measurement into `readings`.
class Sensor {
 public:
  Sensor() = default;
  Sensor(const Sensor&) = delete;
  Sensor& operator=(const Sensor&) = delete;
  virtual ~Sensor() = default;

  virtual const char* Name() const = 0;
  virtual bool Init() = 0;
  virtual bool Read() = 0;
  virtual void Apply(Readings& readings) const = 0;
};

}  // namespace roomsense
