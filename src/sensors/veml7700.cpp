#include "sensors/veml7700.h"

#include <Adafruit_VEML7700.h>
#include <Arduino.h>
#include <Wire.h>

#include "core/readings.h"

namespace roomsense {

const char* Veml7700::Name() const { return "VEML7700"; }

bool Veml7700::Init() {
  Serial.println("Initializing VEML7700");
  if (!veml_.begin(&Wire)) {
    Serial.println("VEML7700 not found");
    return false;
  }
  ok_ = true;
  return true;
}

bool Veml7700::Read() {
  if (!ok_) {
    return false;
  }
  lux_ = veml_.readLux(VEML_LUX_AUTO);
  return true;
}

void Veml7700::Apply(Readings& readings) const {
  readings.lux = lux_;
  readings.veml_ok = ok_;
}

}  // namespace roomsense
